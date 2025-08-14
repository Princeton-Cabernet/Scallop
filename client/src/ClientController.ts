import { Config } from "./Config";
import { P4SFUClient, P4SFUClientStatsMeta } from './P4SFUClient'
import { ReceiverPanel } from './ReceiverPanel';
import { SenderPanel } from './SenderPanel';
import { Util } from './Util'
import { WebRTCStatsReport } from "./WebRTCStatsCollector";

export class ClientController {

    private _p4SFUClient: P4SFUClient;

    private _startVideoButton: HTMLButtonElement;
    private _startAudioButton: HTMLButtonElement;

    private _senderPanel: SenderPanel;
    private _receiverPanels = new Map<number, ReceiverPanel>();

    private _participantId: number | undefined = undefined;

    constructor(configJSONString: string) {

        const config = Config.fromJSONString(configJSONString);

        console.log(`[${Util.time()}] Controller: config=${configJSONString}`);

        console.log(`[${Util.time()}] Controller: Controller(sessionId: ${config.sessionId}, ` +
            `controller: ${config.controller!.host}:${config.controller!.port})`);

        // set up UI elements:

        let videoBtn = document.getElementById("start-video-btn") as HTMLButtonElement;
        let audioBtn = document.getElementById("start-audio-btn") as HTMLButtonElement;

        if (videoBtn && audioBtn) {
            this._startVideoButton = videoBtn;
            this._startAudioButton = audioBtn;
        } else {
            throw Error("could not set up UI elements");
        }

        this._startVideoButton.addEventListener("click", (ev) => this._onClickStartVideo(ev));
        this._startAudioButton.addEventListener("click", (ev) => this._onClickStartAudio(ev));

        this._senderPanel = new SenderPanel("#self .info", "#sender-template");

        // set up callbacks for P4SFUClient:

        this._p4SFUClient = new P4SFUClient(config);

        if (config.telemetry) {
            this._p4SFUClient.enableTelemetry(config.telemetry);
        }

        this._p4SFUClient.onconnected = (sessionId, participantId) =>
            this._onConnected(sessionId, participantId);
        this._p4SFUClient.ondisconnected = () => this._onDisconnected();
        this._p4SFUClient.onsessiondescription = (type, desc) => {
            this._onSessionDescription(type, desc);
        }
        this._p4SFUClient.onstats = (stats) => this._onStats(stats);
        this._p4SFUClient.ontrack = (participantId, stream) => this._onTrack(participantId, stream);
    }

    private _onConnected(sessionId: number, participantId: number) {

        let status = document.getElementById("status");

        this._participantId = participantId;

        if (status != null) {
            status.classList.remove("disconnected");
            status.classList.add("connected");
            status.innerText = `connected / session ${sessionId} / participant ${participantId}`;
        }
    }

    private _onDisconnected() {

        let status = document.getElementById("status");

        if (status != null) {
            status.classList.remove("connected");
            status.classList.add("disconnected");
            status.innerText = "disconnected";
        }
    }

    private _onClickStartVideo(ev: Event) {

        this._p4SFUClient.shareVideo(document.getElementById("video-self") as HTMLVideoElement);
    }

    private _onClickStartAudio(ev: Event) {

        this._p4SFUClient.shareAudio(document.getElementById("audio-self") as HTMLVideoElement);
    }

    private _onSessionDescription(type: "local" | "remote", desc: RTCSessionDescriptionInit) {

        console.log(`[${Util.time()}] Controller: _onSessionDescription(): type=${type}`);
    }

    private _onStats(stats: WebRTCStatsReport<P4SFUClientStatsMeta>) {

         if (stats.candidatePair) {

             let participantId: number = 0;

             if (!stats.meta?.associatedParticipantId) {
                    console.error(`[${Util.time()}] Controller: _onStats(): missing `
                        + `associatedParticipantId`);
                    return;
             } else {
                    participantId = stats.meta.associatedParticipantId;
             }

             if (participantId == this._participantId) {
                 this._senderPanel.update(stats);
             } else {

                 if (this._receiverPanels.has(participantId)) {
                     this._receiverPanels.get(participantId)!.updateStats(stats);
                 } else {
                     console.error(`[${Util.time()}] Controller: _onStats(): could not find panel `
                         + ` for participant ${participantId}`)
                 }
             }
         }
    }

    private async _onTrack(participantId: number, track: MediaStreamTrack) {

        console.log(`[${Util.time()}] Controller: _onTrack(): associatedParticipantId=`
            + `${participantId}, kind=${track.kind}`);

        // if there is no receiver panel for this participant, create one:
        if (!this._receiverPanels.has(participantId)) {
            this._receiverPanels.set(participantId, new ReceiverPanel("#receivers", participantId));
        }

        let stream = new MediaStream();
        stream.addTrack(track);

        let receiverPanel = this._receiverPanels.get(participantId);

        if (!receiverPanel) {
            console.error(`[${Util.time()}] Controller: _onTrack(): could not find receiver panel `
                + `for participant ${participantId}`);
            return;
        }

        if (track.kind == "audio") {
            receiverPanel.audioElement().srcObject = stream;
            receiverPanel.audioElement().muted = false;
            await receiverPanel.audioElement().play();
            console.log(`[${Util.time()}] Controller: _onTrack(): added audio for participant ` +
                `${participantId}`);

        } else if (track.kind == "video") {


            (document.querySelector(`#participant-${participantId} > div.video-view > video`) as HTMLVideoElement)!.srcObject = stream;
            (document.querySelector(`#participant-${participantId} > div.video-view > video`) as HTMLVideoElement)!.muted = true;
            await (document.querySelector(`#participant-${participantId} > div.video-view > video`) as HTMLVideoElement)!.play();
            console.log(`[${Util.time()}] Controller: _onTrack(): set srcObject to #participant-${participantId} > div.video-view > video`);

            // receiverPanel.videoElement().srcObject = stream;
            // receiverPanel.videoElement().muted = true;
            // await receiverPanel.videoElement().play();
            console.log(`[${Util.time()}] Controller: _onTrack(): added video for ` +
                `${participantId}`);
        }
    }

    /*
    private async _onTimer() {
        await this._p4SFUClient.collectStats();
    }
     */
}
