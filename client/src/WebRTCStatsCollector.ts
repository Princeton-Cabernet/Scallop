import { Util } from "./Util";

interface WebRTCStatsSource {
     getStats(): Promise<RTCStatsReport>;
}

export class WebRTCStatsReport<Meta> {
    public meta: Meta | undefined                              = undefined;
    public localCandidate: RTCStats | undefined                = undefined;
    public remoteCandidate: RTCStats | undefined               = undefined;
    public candidatePair: RTCIceCandidatePairStats | undefined = undefined;
    public outboundRTP: RTCOutboundRtpStreamStats[]            = [];
    public inboundRTP: RTCInboundRtpStreamStats[]              = [];
}

export class WebRTCStatsCollector<Meta> {

    private _sources: { source: WebRTCStatsSource, meta: Meta }[] = [];
    private _telemetryEnabled: boolean = false;
    private _telemetryActive: boolean = false;
    private _telemetrySink: { host: string, port: number } | undefined = undefined;
    private _ws: WebSocket | undefined = undefined;

    constructor() { }

    public enableTelemetry(sinkHost: string, sinkPort: number): void {

        this._telemetryEnabled = true;
        this._telemetrySink = { host: sinkHost, port: sinkPort };

        this._ws = new WebSocket(`ws://${sinkHost}:${sinkPort}`);
        this._ws.onopen = () => this._onTelemetryOpen();
        this._ws.onclose = () => this._onTelemetryClose();
        this._ws.onmessage = (event: MessageEvent) => this._onTelemetryMessage(event);
        this._ws.onerror = (event: Event) => this._onTelemetryError(event);
    }

    private _onTelemetryOpen(): void {
        this._telemetryActive = true;
        console.log(`[${Util.time()}] WebRTCStatsCollector: telemetry connection established`);
    }

    private _onTelemetryClose(): void {

        if (this._telemetryActive) {
            console.log(`[${Util.time()}] WebRTCStatsCollector: telemetry connection closed`);
        }

        this._telemetryActive = false;
    }

    private _onTelemetryMessage(event: MessageEvent): void { }

    private _onTelemetryError(event: Event): void {
        console.error(`[${Util.time()}] WebRTCStatsCollector: telemetry error: ${event.type}`);

        if (this._ws!.readyState != WebSocket.OPEN) {
            setTimeout(() => {
                this.enableTelemetry(this._telemetrySink!.host, this._telemetrySink!.port);
            } , 10000);
        }
    }

    public addSource(source: WebRTCStatsSource, meta: Meta): void {
        this._sources.push({source, meta});
    }

    public async collectStats(): Promise<WebRTCStatsReport<Meta>[]> {

        let reports: WebRTCStatsReport<Meta>[] = [];

        for (const source of this._sources) {

            let telemetryReport: WebRTCStatsReport<Meta> = new WebRTCStatsReport<Meta>();
            telemetryReport.meta = source.meta;

            const stats: RTCStatsReport = await source.source.getStats();
            let localCandidateId: string | undefined = undefined;
            let remoteCandidateId: string | undefined = undefined;

            // first collect candidate-pair and rtp stats
            stats.forEach(report => {

                if (report.type === 'candidate-pair' && report.state === 'succeeded') {
                    let candidatePair = report as RTCIceCandidatePairStats;
                    localCandidateId = candidatePair.localCandidateId;
                    remoteCandidateId = candidatePair.remoteCandidateId;
                    telemetryReport.candidatePair = candidatePair;
                } else if (report.type == 'inbound-rtp') {
                    telemetryReport.inboundRTP.push(report as RTCInboundRtpStreamStats);
                } else if (report.type == 'outbound-rtp') {
                    telemetryReport.outboundRTP.push(report as RTCOutboundRtpStreamStats);
                }
            });

            // second retrieve individual candidate pairs based on candidate-pair ids
            if (localCandidateId || remoteCandidateId) {
                stats.forEach(report => {
                    if (report.type === 'local-candidate' && report.id === localCandidateId) {
                        telemetryReport.localCandidate = report;
                    } else if (report.type === 'remote-candidate' && report.id === remoteCandidateId) {
                        telemetryReport.remoteCandidate = report;
                    }
                });
            }

            reports.push(telemetryReport);
        }

        if (this._telemetryActive && this._ws) {
            this._ws!.send(JSON.stringify({reports: reports}));
        }

        return reports;
    }
}
