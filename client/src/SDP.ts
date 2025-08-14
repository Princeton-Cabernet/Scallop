
export namespace SDP {

    export type MediaType = "audio" | "video"
    export type Direction = "sendrecv" | "sendonly" | "recvonly" | "inactive"

    export type MLine = {
        type:         MediaType;
        port:         number;
        protocols:    string;
        payloadTypes: number[];
    }

    export type ICECandidate = {
        foundation:       string;
        componentId:      number;
        protocol:         string;
        priority:         number;
        address:          string;
        port:             number;
        type:             string;
        otherAttributes?: string;
    }

    export type SSRC = {
        ssrc:   number;
        cname?: string;
        msid?:  string;
    }

    export type SSRCGroup = {
        ssrcs: SSRC[];
    }

    const mLineRegex: RegExp          = /m=(audio|video) ([0-9]{1,5}) ([A-Za-z\/]+) ((?:[0-9]{1,3})(?:\s[0-9]{1,3})*)/;
    const cLineRegex: RegExp          = /c=IN IP4 (\d{1,3}.\d{1,3}.\d{1,3}.\d{1,3})/;
    const iceCandidateRegex: RegExp   = /a=candidate:([0-9a-z\/\+]+) (\d{1,3}) (tcp|udp) (\d+) (\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}) (\d{1,5}) typ (host|srflx|prflx|relay)(?:\s?(.*))/;
    const iceUfragRegex: RegExp       = /a=ice-ufrag:([0-9a-zA-Z\/\+]+)/;
    const icePwdRegex: RegExp         = /a=ice-pwd:([0-9a-zA-Z\/\+]+)/;
    const directionRegex: RegExp      = /a=(sendrecv|sendonly|recvonly|inactive)/;
    const midRegex: RegExp            = /a=mid:([0-9a-zA-Z]+)/
    const ssrcLineFilterRegex: RegExp = /a=ssrc/;
    const ssrcGroupRegex: RegExp      = /a=ssrc-group:(FID|FEC) ((?:[0-9]+)(?:\s[0-9]+)+)/;
    const ssrcAttrRegex: RegExp       = /a=ssrc:([0-9]+) (msid|cname):([\w\s\-+]+)/

    export class Header {

        constructor(lines: string[]) {
            this._lines = lines;
        }
    
        public lines(): string[] {
            return this._lines;
        }
    
        private _lines: string[] = [];
    }

    export class MediaDescription {

        constructor(lines: string[]) {
    
            let match: RegExpMatchArray | null;
            let ssrcLines: string[] = [];
    
            for (const line of lines) {
    
                if ((match = line.match(mLineRegex)) && match.length >= 5) {
                    this._parseMLine(match);
                } else if ((match = line.match(cLineRegex)) && match.length > 0) {
                    this._cLine = match[0];
                } else if ((match = line.match(iceCandidateRegex)) && match.length >= 8) {
                    this._parseICECandidate(match);
                } else if ((match = line.match(iceUfragRegex)) && match.length == 2) {
                    this.iceUfrag = match[1];
                } else if ((match = line.match(icePwdRegex)) && match.length == 2) {
                    this.icePwd = match[1];
                } else if ((match = line.match(directionRegex)) && match.length == 2) {
                    this.direction = match[1] as Direction;
                } else if ((match = line.match(midRegex)) && match.length == 2) {
                    this.mid = match[1];
                } else if ((match = line.match(ssrcLineFilterRegex)) && match) {
                    ssrcLines.push(line);
                } else {
                    this._aLines.push(line);
                }
            }

            if (this.direction == "sendonly" || this.direction == "sendrecv") {
                this._parseSSRCLines(ssrcLines);
            }
        }
    
        public lines(): string[] {
    
            let lines: string[] = [];
    
            if (this.mLine)
                lines.push(this._mLineString());
    
            if (this._cLine)
                lines.push(this._cLine);
    
            lines.push(...this._iceCandidateLines());
    
            if (this.iceUfrag)
                lines.push(this._iceUfragLine()!);
    
            if (this.icePwd)
                lines.push(this._icePwdLine()!);
    
            if (this.direction)
                lines.push(this._directionLine()!);
                
            if (this.mid)
                lines.push(this._midLine()!);

            lines.push(...this._aLines);
    
            if (this.ssrcGroup || this.ssrc)
                lines.push(...this._ssrcLines());
    
            return lines;
        }

        public summary(join: string = "\n"): string {

            let out: string = "";

            out += `${this.mid!} ${this.mLine!.type} ${this.direction!} ${this.mLine!.port} ${this.mLine!.protocols.toLowerCase()}${join}`;
            
            for (let c of this.iceCandidates)
                out += ` - ice: ${c.address}:${c.port}/${c.protocol} ${c.type} ${c.priority}${join}`;

            if (this.ssrc) {
                out += ` - ssrc: ${this.ssrc.ssrc}${join}`;
            } else if (this.ssrcGroup) {
                out += ` - ssrcs: ${this.ssrcGroup.ssrcs.map(g => g.ssrc).join(" ")}${join}`;
            }

            return out;
        }
    
        private _parseMLine(match: RegExpMatchArray) {
    
            this.mLine = {
                type:         match[1] as MediaType,
                port:         parseInt(match[2]),
                protocols:    match[3],
                payloadTypes: match[4].split(" ").map((pt) => parseInt(pt))
            }
        }
    
        private _parseICECandidate(match: RegExpMatchArray) {
    
            this.iceCandidates.push({
                foundation: match[1],
                componentId: parseInt(match[2]),
                protocol: match[3],
                priority: parseInt(match[4]),
                address: match[5],
                port: parseInt(match[6]),
                type: match[7],
                otherAttributes: match[8]
            });
        }
    
        private _parseSSRCLines(lines: string[]) {
    
            let match: RegExpMatchArray | null;
            let ssrcGroup: SSRCGroup = {ssrcs: []};
            let singleSSRC: SSRC = {ssrc: 0};
    
            for (const line of lines) {
    
                if ((match = line.match(ssrcGroupRegex)) && match.length == 3) {
                
                    for (const ssrc of match[2].split(" ")) {
                        ssrcGroup.ssrcs.push({
                            ssrc: parseInt(ssrc)
                        })
                    }
    
                } else if ((match = line.match(ssrcAttrRegex)) && match.length == 4) {
    
                    let ssrc = parseInt(match[1]);
                    let groupEntry = ssrcGroup.ssrcs.find((group) => group.ssrc == ssrc);
    
                    if (groupEntry) {
                        if (match[2] == "msid") {
                            groupEntry.msid = match[3];
                        } else if (match[2] == "cname") {
                            groupEntry.cname = match[3];
                        }
                    } else {
                        singleSSRC.ssrc = ssrc;
                        if (match[2] == "msid") {
                            singleSSRC.msid = match[3];
                        } else if (match[2] == "cname") {
                            singleSSRC.cname = match[3];
                        }

                    }
                }
            }
    
            if (singleSSRC.ssrc != 0) {
                this.ssrc = singleSSRC;
            } else if (ssrcGroup.ssrcs.length > 0) {
                this.ssrcGroup = ssrcGroup;
            }
        }
    
        private _mLineString(): string {
    
            if (this.mLine) {
                return `m=${this.mLine.type} ${this.mLine.port} ${this.mLine.protocols} `
                    + `${this.mLine.payloadTypes.join(" ")}`;
            }
    
            throw Error("SDP.MediaDescription: no m-line");
        }
    
        private _iceUfragLine(): string | undefined {
            
            if (this.iceUfrag) {
                return `a=ice-ufrag:${this.iceUfrag}`;
            }
        }
    
        private _icePwdLine(): string | undefined {
            
            if (this.icePwd) {
                return `a=ice-pwd:${this.icePwd}`;
            }
        }
    
        private _iceCandidateLines(): string[] {
    
            let lines: string[] = [];
    
            for (const c of this.iceCandidates) {
                lines.push(`a=candidate:${c.foundation} ${c.componentId} ${c.protocol} ${c.priority} `
                    + `${c.address} ${c.port} typ ${c.type} ${c.otherAttributes}`);
            }
    
            return lines;
        }
    
        private _directionLine(): string | undefined {
        
            if (this.direction) {
                return `a=${this.direction}`;
            }
        }

        private _midLine(): string | undefined {

            if (this.mid)
                return `a=mid:${this.mid}`;
        }
    
        private _ssrcLines(): string[] {
    
            let res: string[] = [];
    
            if (this.ssrcGroup) {
    
                let ssrcs: number[] = this.ssrcGroup.ssrcs.map((group) => group.ssrc);
                res.push(`a=ssrc-group:FID ${ssrcs.join(" ")}`);
    
                for (const ssrc of this.ssrcGroup.ssrcs) {
                    
                    if (ssrc.cname)
                        res.push(`a=ssrc:${ssrc.ssrc} cname:${ssrc.cname}`);
    
                    if (ssrc.msid)
                        res.push(`a=ssrc:${ssrc.ssrc} msid:${ssrc.msid}`);
                }

            } 
            
            if (this.ssrc) {
                if (this.ssrc!.cname)
                    res.push(`a=ssrc:${this.ssrc.ssrc} cname:${this.ssrc.cname}`);

                if (this.ssrc!.msid)
                    res.push(`a=ssrc:${this.ssrc.ssrc} msid:${this.ssrc.msid}`);

                if (this.ssrc!.cname && this.ssrc!.msid)
                    res.push(`a=ssrc:${this.ssrc.ssrc}`);
            }
    
            return res;
        }
    
    
        public iceUfrag?: string;
        public icePwd?: string;
        public iceCandidates: ICECandidate[] = [];
        public direction?: Direction;
        public mid?: string;
        public mLine?: MLine;
        public ssrc?: SSRC;
        public ssrcGroup?: SSRCGroup;

        private _cLine: string = "";
        private _aLines: string[] = [];
    }

    export class SessionDescription {

        // https://datatracker.ietf.org/doc/html/rfc4566#section-5
    
        private static _newLineRegex: RegExp = /\r\n/;
        private static _mLineRegex: RegExp = /m=(audio|video) ([0-9]{1,5}) ([A-Za-z\/]+) ((?:[0-9]{1,3})(?:\s[0-9]{1,3})*)/;
    
        public header: Header;
        public mediaDescriptions: MediaDescription[] = [];
    
        constructor(sdp: string) {
    
            const lines = sdp.split(SessionDescription._newLineRegex);
            let currentMediaSection = -1;
            let mediaSectionCount = 0;
            let mediaSectionLines: string[] = [];
            let headerLines: string[] = [];
    
            for (let i = 0; i < lines.length; i++) {
    
                // parse new media line
                if (lines[i].match(SessionDescription._mLineRegex)) {
                    currentMediaSection = mediaSectionCount;
                }
    
                if (currentMediaSection >= 0) {
                                    
                    if (lines[i] != "") {
                        mediaSectionLines.push(lines[i]);
                    }
    
                    // if at end of all lines or next line is m-line, then end current media section
                    if ((i == lines.length-1) || (i < lines.length-1 && lines[i+1].match(SessionDescription._mLineRegex))) {
    
                        this.mediaDescriptions.push(new MediaDescription(mediaSectionLines));
    
                        currentMediaSection = -1;
                        mediaSectionCount++;
                        mediaSectionLines = [];
                    }
                } else {
    
                    if (lines[i] != "") {
                        headerLines.push(lines[i]);
                    }
                }
            }
    
            this.header = new Header(headerLines);
        }
    
        public lines(): string[] {
    
            let lines: string[] = [];
    
            lines.push(...this.header.lines());
    
            for (const mediaDescription of this.mediaDescriptions) {
                lines.push(...mediaDescription.lines());
            }
    
            return lines;
        }
    
        public string(join: string = "\r\n"): string {
            return this.lines().join(join);
        }

        public summary(join: string = "\n"): string {

            let out: string = "";

            for (const media of this.mediaDescriptions) {
                out += `${media.summary(join)}${join}`;
            }

            return out;
        }
    }
}
