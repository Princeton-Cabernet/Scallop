import { TelemetryReport} from "./TelemetrySender";
import Mustache from "mustache";
import {P4SFUClientTelemetryMeta} from "./P4SFUClient";

export class ReceiverPanel {

    constructor(parentSelector: string, participantId: number) {

        this._participantId = participantId;

        let panelTemplate = "";
        let parentElement: HTMLDivElement | null = null;

        if (document.querySelector("#receiver-panel-tpl")) {
            panelTemplate = document.querySelector("#receiver-panel-tpl")!.innerHTML.trim();
        } else {
            throw Error(`ReceiverPanel: could not find selector #receiver-panel-tpl`);
        }

        if (document.querySelector("#receiver-info-tpl")) {
            this._infoTemplate = document.querySelector("#receiver-info-tpl")!.innerHTML.trim();
        } else {
            throw Error(`ReceiverPanel: could not find selector #receiver-info-tpl`);
        }

        if (document.querySelector(parentSelector)) {
            parentElement = document.querySelector(parentSelector);
        } else {
            throw Error(`ReceiverPanel: could not find selector ${parentElement}`);
        }

        parentElement!.insertAdjacentHTML("beforeend", Mustache.render(panelTemplate, {
            participantId: participantId
        }));

        this.videoSelector = `#participant-${participantId} > div.video-view > video`;
        this.audioSelector = `#participant-${participantId} > div.video-view > audio`;
        this.infoSelector  = `#participant-${participantId} > div.info`;
    }

    public updateStats(stats: TelemetryReport<P4SFUClientTelemetryMeta>) {

        document.querySelector(this.infoSelector)!.innerHTML
            = Mustache.render(this._infoTemplate, stats);
    }

    public videoElement(): HTMLVideoElement {
        return document.querySelector(this.videoSelector) as HTMLVideoElement;
    }

    public audioElement(): HTMLAudioElement {
        return document.querySelector(this.audioSelector) as HTMLAudioElement;
    }

    public readonly videoSelector: string;
    public readonly audioSelector: string;
    public readonly infoSelector: string;

    private readonly _infoTemplate: string = "";
    private readonly _participantId: number;
}
