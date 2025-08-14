
import {AnswerMessage, OfferMessage, SignalClient} from './SignalClient';
import { Util } from './Util';
import * as console from "console";
import { WebRTCStatsCollector, WebRTCStatsReport } from "./WebRTCStatsCollector";
import {Config} from "../shared/Config";

export class P4SFUClientStatsMeta {
    public fromParticipantId: number | undefined       = undefined;
    public meetingId: number | undefined               = undefined;
    public associatedParticipantId: number | undefined = undefined;
}

class MediaConnection {

    //#region Events
    public onlocaldescription?: (desc: RTCSessionDescription) => void;
    public ontrack?: (connection: MediaConnection, track: MediaStreamTrack) => void;
    //#endregion

    constructor(type: "recv" | "send", associatedParticipantId: number, config: Config) {

        console.log(`[${Util.time()}] MediaConnection: MediaConnection(`
            + `type: ${type}, associatedParticipantId: ${associatedParticipantId})`);

        this._type = type;
        this._associatedParticipantId = associatedParticipantId;
        this._config = config;

        this._pc = new RTCPeerConnection({
            // send all peer connections via single UDP flow
            bundlePolicy: "max-bundle",
            // prefetch up to 8 ICE candidates
            iceCandidatePoolSize: 8,
            // require RTCP multiplexing (rtcp:9)
            rtcpMuxPolicy: "require",
            // do not use external STUN server -> only local candidates
            iceServers: [],
            // use unified plan SDP semantics
            // @ts-ignore (only available in Chrome)
            sdpSemantics: "unified-plan"
        });

        this._pc.onsignalingstatechange     = (e) => this._onSignalingOrICEGatheringStateChange(e);
        this._pc.onicegatheringstatechange  = (e) => this._onSignalingOrICEGatheringStateChange(e);
        this._pc.oniceconnectionstatechange = (e) => this._onICEConnectionStateChange(e);
        this._pc.onicecandidate             = (e) => this._onICECandidate(e);
        this._pc.onicecandidateerror        = (e) => this._onICECandidateError(e)
        this._pc.ontrack                    = (e) => this._onTrack(e);
        this._pc.onconnectionstatechange    = (e) => this._onConnectionStateChange(e);
        this._pc.onnegotiationneeded        = (e) => this._onNegotiationNeeded(e);

        /*
        this._pc.onicecandidate = (e: RTCPeerConnectionIceEvent) => {

            if (e.candidate.protocol == "udp") {
                console.log("udp ice candidate");
            } else if (e.candidate.protocol == "tcp") {
                console.log("tcp ice candidate");
                return;
            }

            if (e.candidate !== null) {
                console.log(`[${Util.time()}] MediaConnection: _onICECandidate(): ` +
                    `${e.candidate.address}:${e.candidate.port} (${e.candidate.protocol})`);
            } else {
                console.log(`[${Util.time()}] MediaConnection: _onICECandidate(): null`);
            }


        }
        */
    }

    public async addSendStream(stream: MediaStream) {

        console.log(`[${Util.time()}] MediaConnection: addSendStream()`);

        if (this._type == "recv") {
            console.error(`[${Util.time()}] MediaConnection: addSendStream(): cannot add `
                + `send stream to recv connection`);
            return;
        }

        for (const track of stream.getTracks()) {

            if (track.kind == "video") {

                if (!this._videoActive) {

                    let transceiver = this._pc.addTransceiver(track, {direction: "sendonly"});
                    transceiver.setCodecPreferences(
                        MediaConnection._preferCodecs("video", ["video/av1", "video/rtx"]));
                    this._videoActive = true;
                } else {
                    console.error(`[${Util.time()}] MediaConnection: addSendStream(): already `
                        + `sending video`);
                }

            } else if (track.kind == "audio") {

                if (!this._audioActive) {

                    let transceiver = this._pc.addTransceiver(track, {direction: "sendonly"});
                    transceiver.setCodecPreferences(
                        MediaConnection._preferCodecs("audio", ["audio/opus"]));
                    this._audioActive = true;
                } else {
                    console.error(`[${Util.time()}] MediaConnection: addSendStream(): already `
                        + `sending audio`);
                }
            }
        }

        let offer = await this._pc.createOffer({
            offerToReceiveAudio: false,
            offerToReceiveVideo: false
        });

        await this._pc.setLocalDescription(offer);
    }

    public async setRemoteDescription(desc: RTCSessionDescriptionInit) {

        console.log(`[${Util.time()}] MediaConnection: setRemoteDescription()`);

        try {
            await this._pc.setRemoteDescription(desc);

            if (desc.type == "answer" && this._type == "send") {

                const scalabilityMode = "L1T3";

                try {
                    for (let sender of this._pc.getSenders()) {
                        let params = sender.getParameters();
                        (params as RTCRtpSendParameters).encodings.forEach(e => {
                            //@ts-ignore --- ignore error regarding scalability mode API
                            e.scalabilityMode = scalabilityMode;
                        });

                        await sender.setParameters(params);
                        console.log(`[${Util.time()}] MediaConnection: setAnswer(): scalability `
                            + `mode set to ${scalabilityMode}`);
                    }
                } catch (err) {
                    console.error(`[${Util.time()}] MediaConnection: setAnswer(): failed setting `
                        + `scalability: ${(err as Error).name} - ${(err as Error).message}`);
                }
            } else {
                console.debug(`[${Util.time()}] MediaConnection: setRemoteDescription():`
                    + ` not setting scalability mode for type ${desc.type}`);
            }

        } catch (e) {
            console.error(e);
        }
    }

    public async createAnswer(): Promise<RTCSessionDescriptionInit> {

        let answer = await this._pc.createAnswer();
        await this._pc.setLocalDescription(answer);
        return answer;
    }

    public signalingState(): RTCSignalingState {
        return this._pc.signalingState;
    }

    public async getStats(): Promise<RTCStatsReport> {

        return await this._pc.getStats();
    }

    public type(): "send" | "recv" | undefined {
        return this._type;
    }

    public videoActive(): boolean {
        return this._videoActive;
    }

    public audioActive(): boolean {
        return this._audioActive;
    }

    public associatedParticipantId(): number {
        return this._associatedParticipantId;
    }

    public stop() {
        this._pc.close();
    }

    private _offerAvailableForSignaling(): boolean {

        return this._pc.iceGatheringState     == "complete"
            && this._pc.signalingState        == "have-local-offer"
            && this._pc.localDescription      != null
            && this._pc.localDescription.type == "offer";
    }

    private _answerAvailableForSignaling(): boolean {

        return this._pc.iceGatheringState     == "complete"
            && this._pc.signalingState        == "stable"
            && this._pc.localDescription      != null
            && this._pc.localDescription.type == "answer";
    }

    private async _onSignalingOrICEGatheringStateChange(event: Event) {

        console.log(`[${Util.time()}] MediaConnection: _onSignalingOrICEGatheringStateChange(): `
            + `iceGatheringstate=${this._pc.iceGatheringState}, `
            + `signalingState=${this._pc.signalingState}`);

        if (this._offerAvailableForSignaling() || this._answerAvailableForSignaling()) {

            console.log(`[${Util.time()}] MediaConnection: _onSignalingOrICEGatheringStateChange(): `
                + `have description for signaling: type=${this._pc.localDescription!.type}`);


            console.log(this._pc.localDescription!.sdp);


            if (this.onlocaldescription && this._pc.localDescription) {

                if (this._config.limitip != undefined) {

                    console.log(`[${Util.time()}] MediaConnection: limitip: ${this._config.limitip}`);

                    const type = this._pc.localDescription.type;

                    let localDescription = [];

                    for (let m of this._pc.localDescription.sdp.split("\r\n")) {

                        if (m.startsWith("a=candidate:")) {
                            if (!m.includes(this._config.limitip)) {
                                console.log(`[${Util.time()}] MediaConnection: skip candidate: ${m}`);
                                continue;
                            }
                        }

                        localDescription.push(m);
                    }

                    // set the local description again only if it is an offer, otherwise send the
                    // answer limited to the permissible IP addresses directly to controller
                    if (type == "offer") {

                        await this._pc.setLocalDescription({
                            type: type,
                            sdp: localDescription.join("\r\n")
                        });

                        console.log("local desc 2:");
                        console.log(this._pc.localDescription.sdp);
                        this.onlocaldescription(this._pc.localDescription);

                    } else if (type == "answer") {

                        console.log("local desc 2:");
                        console.log(localDescription.join("\r\n"));

                        this.onlocaldescription({
                            type: "answer",
                            sdp: localDescription.join("\r\n")
                        } as RTCSessionDescription);
                    }
                } else {

                    this.onlocaldescription(this._pc.localDescription);



                }

            } else {
                console.error(`[${Util.time()}] MediaConnection: _onSignalingOrICEGatheringStateChange():`
                    + ` onlocaldescription not set or localDescription is undefined`);
            }
        }
    }

    private _onICEConnectionStateChange(e: Event) {
        console.log(`[${Util.time()}] MediaConnection: _onICEConnectionStateChange(): `
            + `iceConnectionState=${this._pc.iceConnectionState}`);
    }

    private _onICECandidate(e: RTCPeerConnectionIceEvent) {

        if (e.candidate !== null) {
            console.log(`[${Util.time()}] MediaConnection: _onICECandidate(): ` +
                `${e.candidate.address}:${e.candidate.port} (${e.candidate.protocol})`);
        } else {
            console.log(`[${Util.time()}] MediaConnection: _onICECandidate(): null`);
        }
    }

    private _onICECandidateError(e: RTCPeerConnectionIceErrorEvent) {
        console.log(`[${Util.time()}] MediaConnection: _onICECandidateError(): ` + e.errorText);
    }

    private _onTrack(e: RTCTrackEvent) {

        console.log(`[${Util.time()}] MediaConnection: _onTrack(): `
            + `kind=${e.track.kind}, id=${e.track.id}`);

        if (this.ontrack) {
            this.ontrack(this, e.track);
        } else {
            console.error(`[${Util.time()}] MediaConnection: _onTrack(): ontrack not set`);
        }
    }

    private _onConnectionStateChange(e: Event) {
        console.log(`[${Util.time()}] MediaConnection: _onConnectionStateChange(): `
            + `connectionState=${this._pc.connectionState}`);
    }

    private _onNegotiationNeeded(e: Event) {
        console.log(`[${Util.time()}] MediaConnection: _onNegotiationNeeded()`);
    }

    private static _preferCodecs(kind: "video" | "audio", mimeTypes: string[])
        : RTCRtpCodecCapability[] {

        return RTCRtpReceiver.getCapabilities(kind)!.codecs.filter(c => {
            return mimeTypes.includes(c.mimeType.toLowerCase());
        });
    };

    private _pc: RTCPeerConnection;
    private readonly _type?: "send" | "recv" = undefined;
    private _videoActive: boolean = false;
    private _audioActive: boolean = false;
    private readonly _associatedParticipantId: number;
    private _config: Config;
}

export class P4SFUClient {

    //#region Events
    public onconnected?: (sessionId: number, participantId: number) => void;
    public ondisconnected?: () => void;
    public onsessiondescription?: (type: "local" | "remote",
                                   desc: RTCSessionDescriptionInit) => void;
    public onstats?: (stats: WebRTCStatsReport<P4SFUClientStatsMeta>) => void;
    public ontrack?: (participantId: number, track: MediaStreamTrack) => void;
    //#endregion

    constructor(config: Config) {

        console.log(`[${Util.time()}] P4SFUClient: P4SFUClient(config: ${config})`);
        this._sessionId = config.sessionId;
        this._config = config;

        this._signalClient = new SignalClient(config.controller.host, config.controller.port, config.sessionId);

        this._signalClient.onconnected = (sessionId: number, participantId: number) => {
            this._onConnected(sessionId, participantId);
        };

        this._signalClient.ondisconnected = () => {
            this._onDisconnected();
        };

        this._signalClient.onoffer = (msg: OfferMessage) => {
            this._onRemoteOffer(msg);
        };

        this._signalClient.onanswer = async (msg: AnswerMessage) => {
            await this._onRemoteAnswer(msg);
        };

        this._statsCollector = new WebRTCStatsCollector<P4SFUClientStatsMeta>();

        setInterval(async () => await this.collectStats(), 500);
    }

    public enableTelemetry(config: {host: string, port: number}) {
        if (config.host && config.port) {
            console.log(`[${Util.time()}] P4SFUClient: enableTelemetry(): `
                + `sink=${config.host}:${config.port}`);
            this._statsCollector.enableTelemetry(config.host, config.port);
        }
    }

    public async shareVideo(videoElement: HTMLVideoElement) {

        if (!this._sendMediaConnection || !this._sendMediaConnection.videoActive()) {

            let stream = await this._getMedia({
                audio: false,
                video: { width: { exact: 1280 }, height: { exact: 720 }}
            });

            if (stream) {
                videoElement.srcObject = stream;
            }

        } else {
            console.error("P4SFUClient: shareVideo(): already sharing video");
        }
    }

    public async shareAudio(audioElement: HTMLAudioElement) {

        if (!this._sendMediaConnection || !this._sendMediaConnection.audioActive()) {
            let stream = await this._getMedia({video: false, audio: true});

            if (stream) {
                audioElement.srcObject = stream;
            }
        } else {
            console.error("P4SFUClient: shareVideo(): already sharing audio");
        }
    }

    public async collectStats(): Promise<void> {

        const stats = await this._statsCollector.collectStats();

        if (this.onstats) {
            for (const report of stats) {
                this.onstats(report);
            }
        }
    }

    public signalingActive(): boolean {

        if (this._sendMediaConnection) {
            return this._sendMediaConnection.signalingState() != "stable";
        }

        return !![...this._receiveMediaConnections.values()].filter(
            (c: MediaConnection) => c.signalingState() != "stable");
    }
    
    private async _getMedia(constraints: MediaStreamConstraints): Promise<MediaStream | undefined> {

        if (!this._participantId) {
            console.error("P4SFUClient: _getMedia(): client not connected to controller");
            return;
        }

        try {

            if (this._sendMediaConnection == undefined) {
                this._sendMediaConnection = new MediaConnection("send", this._participantId!, this._config);
                this._sendMediaConnection.onlocaldescription = (d) => this._onLocalDescription(d);

                this._statsCollector.addSource(this._sendMediaConnection, {
                    associatedParticipantId: this._participantId,
                    meetingId: this._sessionId,
                    fromParticipantId: this._participantId
                });
            }

            let stream = await navigator.mediaDevices.getUserMedia(constraints);
            await this._sendMediaConnection.addSendStream(stream);
            return stream;

        } catch (e) {
            console.log(e);
        }
    }

    private _mediaConnectionForParticipantId(participantId: number): MediaConnection | undefined {

        if (participantId == this._participantId) {
            return this._sendMediaConnection;
        }

        return this._receiveMediaConnections.get(participantId);
    }

    private _onConnected(sessionId: number, participantId: number) {

        console.log(`[${Util.time()}] P4SFUClient: _onConnected(): sessionId: ${sessionId}, `
            + `participantId: ${participantId}`);

        this._participantId = participantId;

        if (this.onconnected) {
            this.onconnected(sessionId, participantId);
        }
    }

    private _onDisconnected() {

        console.log(`[${Util.time()}] P4SFUClient: _onDisconnected()`);

        if (this._sendMediaConnection) {
            this._sendMediaConnection.stop();
        }

        for (let mc of this._receiveMediaConnections.values()) {
            mc.stop();
        }

        if (this.ondisconnected) {
            this.ondisconnected();
        }
    }

    private async _onRemoteOffer(msg: OfferMessage) {

        console.log(`[${Util.time()}] P4SFUClient: _onRemoteOffer()`);
        console.debug(msg);

        if (!this._mediaConnectionForParticipantId(msg.participant_id)) {

            let mc = new MediaConnection("recv", msg.participant_id, this._config);
            mc.onlocaldescription = (desc: RTCSessionDescription) => this._onLocalDescription(desc);
            mc.ontrack = (c: MediaConnection, t: MediaStreamTrack) => this._onTrack(c, t);

            this._statsCollector.addSource(mc, {
                associatedParticipantId: msg.participant_id,
                fromParticipantId: this._participantId,
                meetingId: this._sessionId
            });

            this._receiveMediaConnections.set(msg.participant_id, mc);

            console.log(`[${Util.time()}] P4SFUClient: _onRemoteOffer(): added media connection ` +
                `for participant ${msg.participant_id}`);
        }

        let mc = this._mediaConnectionForParticipantId(msg.participant_id)!;
        await mc.setRemoteDescription({ type: "offer", sdp: msg.sdp });
        await mc.createAnswer();
    }

    private async _onRemoteAnswer(msg: AnswerMessage) {

        console.log(`[${Util.time()}] P4SFUClient: _onRemoteAnswer()`);
        console.debug(msg);

        if (this._sendMediaConnection) {
            await this._sendMediaConnection.setRemoteDescription({type: "answer", sdp: msg.sdp!});

            if (this.onsessiondescription) {
                this.onsessiondescription("remote", {type: "answer", sdp: msg.sdp!});
            } else {
                console.error(`[${Util.time()}] P4SFUClient: _onRemoteAnswer():`
                    + `onsessiondescription handler not set`);
            }
        } else {
            console.error(`[${Util.time()}] P4SFUClient: _onRemoteAnswer(): no send connection`);
        }
    }

    private _onLocalDescription(desc: RTCSessionDescription) {

        console.log(`[${Util.time()}] P4SFUClient: _onLocalDescription(): type=${desc.type}`);
        console.debug(desc.sdp);

        if (desc.type == "offer") {
            this._signalClient.sendOffer(desc);
        } else if (desc.type == "answer") {
            this._signalClient.sendAnswer(desc);
        }

        if (this.onsessiondescription) {
            this.onsessiondescription("local", desc);
        }
    }

    private _onTrack(c: MediaConnection, t: MediaStreamTrack) {

        console.log(`[${Util.time()}] P4SFUClient: _onTrack()`);

        if (this.ontrack) {
            this.ontrack(c.associatedParticipantId(), t);
        } else {
            console.error(`[${Util.time()}] P4SFUClient: _onTrack(): ontrack handler not set`);
        }
    }

    private _sendMediaConnection?: MediaConnection;
    private _receiveMediaConnections = new Map<number, MediaConnection>();
    private _signalClient: SignalClient;
    private _sessionId;
    private _participantId: number | undefined;
    private _statsCollector: WebRTCStatsCollector<P4SFUClientStatsMeta>;
    private _config: Config;
}
