
import { Util } from './Util';

export interface OfferMessage {
    sdp: string;
    participant_id: number;
}

export interface AnswerMessage {
    sdp: string;
    participant_id: number;
}

export class SignalClient {

    //#region Events
    public onanswer?: (msg: AnswerMessage) => void;
    public onoffer?: (msg: OfferMessage) => void;
    public onconnected?: (sessionId: number, participantId: number) => void;
    public ondisconnected?: () => void;
    //#endregion

    constructor(ip: string, port: number, sessionId: number) {

        console.log(`[${Util.time()}] SignalClient: SignalClient(ip: ${ip}, port: ${port}, `
            + `sessionId: ${sessionId})`);

        this._sessionId = sessionId;
        this._participantId = -1;

        this._ws = new WebSocket(`wss://${ip}:${port}`)
        this._ws.onopen = () => this._onOpen();
        this._ws.onclose = () => this._onClose();
        this._ws.onmessage = (event: MessageEvent) => this._onMessage(event);
        this._ws.onerror = (event: Event) => this._onError(event);
    }
    public sendOffer(desc: RTCSessionDescription) {

        console.log(`[${Util.time()}] SignalClient: sendOffer()`);

        if (this._established) {
            
            if (desc.type != "offer") {
                console.error(`[${Util.time()}] SignalClient: sendOffer(): invalid message`);
            } else {
                this._ws.send(JSON.stringify({
                    type: "offer",
                    data: {
                        sdp: desc.sdp,
                        participant_id: this._participantId
                    }
                }));
            }
        } else {
            console.error(`[${Util.time()}] SignalClient: sendOffer(): `
                + `connection is not established`);
        }
    }
    public sendAnswer(desc: RTCSessionDescriptionInit) {

        console.log(`[${Util.time()}] SignalClient: sendAnswer()`);

        if (this._established) {

            if (desc.type != "answer") {
                console.error(`[${Util.time()}] SignalClient: sendAnswer(): invalid message`);
            } else {
                this._ws.send(JSON.stringify({
                    type: "answer",
                    data: {
                        sdp: desc.sdp,
                        participant_id: this._participantId
                    }
                }));
            }

        } else {
            console.error(`[${Util.time()}] SignalClient: sendAnswer(): `
                + `connection is not established`);
        }
    }

    public sendHello() {
        this._ws.send(JSON.stringify({type: "hello", data: {session_id: this._sessionId}}));
        console.log(`[${Util.time()}] SignalClient: sendHello(): sent hello`);
    }

    private _onOpen() {
        console.log(`[${Util.time()}] SignalClient: _onOpen()`);
        this.sendHello();
    }

    private _onClose() {
        console.log(`[${Util.time()}] SignalClient: _onClose()`);
        this._established = false;

        if (this.ondisconnected) {
            this.ondisconnected();
        }
    }

    private _onMessage(event: MessageEvent) {

        this._readBuffer += event.data;
        this._readBufferLength += event.data.length;
        console.log(`[${Util.time()}] SignalClient: _onMessage(): `
            + `buffered ${event.data.length} bytes`);

        let msg: any = undefined;

        try {
            msg = JSON.parse(this._readBuffer);
            this._readBuffer = "";
            this._readBufferLength = 0;
            console.log(`[${Util.time()}] SignalClient: _onMessage(): successfully parsed buffer`);
        } catch (error) {
            console.log(`[${Util.time()}] SignalClient: _onMessage(): failed parsing buffer`);
            return;
        }

        switch (msg.type) {

            case "hello":   { this._onHello(msg); break; }
            case "answer":  { this._onAnswer(msg); break; }
            case "offer":   { this._onOffer(msg); break; }
            
            default: {
                console.log(`[${Util.time()}] SignalClient: _onMessage(): unknown message type`);
            }
        }
    }

    private _onHello(message: any) {

        console.log(`[${Util.time()}] SignalClient: _onHello()`);
        this._established = true;

        if (message.data.participant_id) {
            this._participantId = message.data.participant_id;
        } else {
            console.error(`[${Util.time()}] SignalClient: _onHello(): no participant id`);
        }

        if (this.onconnected) {
            this.onconnected(this._sessionId, this._participantId);
        }
    }

    private _onAnswer(msg: any) {

        console.log(`[${Util.time()}] SignalClient: _onAnswer()`);
        console.debug(msg);

        if ('sdp' in msg.data && 'participant_id' in msg.data) {
            if (this.onanswer) {
                this.onanswer(msg.data);
            } else {
                console.error(`[${Util.time()}] SignalClient: _onAnswer(): no onAnswer receiver`);
            }
        } else {
            console.error(`[${Util.time()}] SignalClient: _onAnswer(): invalid message`);
        }
    }

    private _onOffer(msg: any) {

        console.log(`[${Util.time()}] SignalClient: _onOffer()`);
        console.debug(msg);

        if ('sdp' in msg.data && 'participant_id' in msg.data) {
            if (this.onoffer) {
                this.onoffer(msg.data);
            } else {
                console.error(`[${Util.time()}] SignalClient: _onOffer(): no onAnswer receiver`);
            }
        } else {
            console.error(`[${Util.time()}] SignalClient: _onOffer(): invalid message`);
        }
    }

    private _onError(event: Event) {

        console.error(`[${Util.time()}] SignalClient: _onError()`);
        console.error(event);
    }

    private _ws: WebSocket;
    private _established: boolean = false;
    private _sessionId: number;
    private _participantId: number;

    private _readBuffer: string = "";
    private _readBufferLength: number = 0;
}
