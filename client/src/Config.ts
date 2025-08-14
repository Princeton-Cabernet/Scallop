
export class Config {
    constructor(sessionId: number, controller = {host: undefined, port: undefined},
                stun = undefined, telemetry = undefined, limitip = undefined) {

        this.sessionId  = sessionId;
        this.controller = controller;
        this.stun       = stun;
        this.telemetry  = telemetry;
        this.limitip    = limitip;
    }

    public sessionId: number;
    public controller: {host: string | undefined, port: number | undefined};
    public stun: {host: string, port: number} | undefined;
    public telemetry: {host: string, port: number} | undefined;
    public limitip: string | undefined;

    public toJSONString(): string {
        return JSON.stringify(this);
    }

    static fromJSONString(jsonString: string): Config {

        let c: Config = new Config(0);
        let parsed;

        try {
            parsed = JSON.parse(jsonString);
        } catch (e) {
            throw Error("could not parse config JSON string");
        }

        // session id is required
        if (parsed.hasOwnProperty("sessionId") && parsed["sessionId"] !== undefined) {
            c.sessionId = parseInt(parsed["sessionId"]);
        } else {
            throw Error("missing property: sessionId");
        }

        // controller is optional but has a default value
        if (parsed.hasOwnProperty("controller") && parsed["controller"] !== undefined) {
            c.controller = parsed["controller"];
        } else {
            c.controller = {host: "127.0.0.1", port: 3301};
        }

        // controller is optional but has a default value
        if (parsed.hasOwnProperty("stun") && parsed["stun"] !== undefined) {
            c.stun = parsed["stun"];
        } else {
            c.stun = undefined;
        }

        // telemetry is optional and has no default value
        if (parsed.hasOwnProperty("telemetry") && parsed["telemetry"] !== undefined) {
            c.telemetry = parsed["telemetry"];
        } else {
            c.telemetry = undefined;
        }

        // limitip is optional and has no default value
        if (parsed.hasOwnProperty("limitip") && parsed["limitip"] !== undefined) {
            c.limitip = parsed["limitip"];
        } else {
            c.limitip = undefined;
        }

        return c;
    }
}