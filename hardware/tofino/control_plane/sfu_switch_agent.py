#!/usr/bin/env python3

import argparse
import asyncio
import websockets
import json
import ports
import os
from enum import Enum

########################################

# Reserved SSRC to indicate no retransmission stream. Relevant for non-video streams.
NO_RTX_SSRC = 0

# Three media types defined
Media = Enum("Media", "video audio screenshare")

# Three quality levels defined
Quality = Enum("Quality", "base mid high") # Max. 3 temporal layers, 1 spatial layer

########################################

class Meeting:
    """
    Data-only class to maintain the state of all active meetings
    """
    def __init__(self, meeting_id) -> None:
        self.meeting_id = meeting_id
    
    def display(self):
        print(f"  Meeting: ID {self.meeting_id}")

########################################

class Participant:
    """
    Data-only class to maintain the state of all active participants
    """
    def __init__(self, meeting_id, ip, port, eport) -> None:
        self.meeting_id  = meeting_id # Meeting this participant is associated with
        self.ip          = ip         # IP of the participant
        self.port        = port       # Port number of the participant
        self.egress_port = eport      # Egress port number of the participant
    
    def display(self):
        print(f"    Participant: [ID {self.meeting_id}]: IP {self.ip}, port {self.port}, egress port {self.egress_port}")

########################################

class SendStream:
    """
    Data-only class to maintain the state of all active ingress streams
    """
    def __init__(self, meeting_id, sip, sport, ssrc, ssrc_rtx, media_type) -> None:
        self.meeting_id  = meeting_id   # Meeting ID this stream is associated with
        self.source_ip   = sip          # IP address of the participant this stream originates from
        self.source_port = sport        # Port of the participant this stream originates from
        self.ssrc        = ssrc         # SSRC of the stream
        self.ssrc_rtx    = ssrc_rtx    # SSRC of the associated retransmission stream
        self.media_type  = media_type   # Media type of the stream (video/audio/screenshare)
    
    def display(self):
        print(f"      Send stream: [ID {self.meeting_id}], [IP {self.source_ip}, port {self.source_port}]: " \
              + f"SSRC {self.ssrc}, rtx. SSRC {self.ssrc_rtx}, type {self.media_type}")

########################################

class ReceiveStream:
    """
    Data-only class to maintain the state of all active egress streams
    """
    def __init__(self, meeting_id, sip, sport, dip, dport, ssrc, ssrc_rtx, eport, media_type, quality = Quality.high) -> None:
        self.meeting_id  = meeting_id   # Meeting ID this stream is associated with
        self.source_ip   = sip          # IP address of the participant this stream originates from
        self.source_port = sport        # Port of the participant this stream originates from
        self.dest_ip     = dip          # IP of the participant this stream terminates at
        self.dest_port   = dport        # Port of the participant this stream terminates at
        self.ssrc        = ssrc         # SSRC of the stream
        self.ssrc_rtx    = ssrc_rtx    # SSRC of the associated retransmission stream
        self.media_type  = media_type   # Media type of the stream (video/audio/screenshare)
        self.quality     = quality      # Media quality (high/mid/base), default is high
        self.egress_port = eport        # Egress port of the stream
    
    def key(self):
        return (self.meeting_id, self.source_ip, self.source_port, self.dest_ip, self.dest_port, self.ssrc)

    def key_rtx(self):
        return (self.meeting_id, self.source_ip, self.source_port, self.dest_ip, self.dest_port, self.ssrc_rtx)
    
    def get_api_message(self, message_type):
        api_message = None
        
        if message_type == "add_stream":
            api_message = {
                    "api": "add_stream",
                    "mid": self.meeting_id,
                    "sip": self.source_ip, "sport": self.source_port,
                    "ssrc": self.ssrc, "ssrc_rtx": self.ssrc_rtx,
                    "dip": self.dest_ip, "dport": self.dest_port,
                    "eport": self.egress_port
                }
            
        elif message_type == "remove_stream":
            api_message = {
                "api": "remove_stream",
                "mid": self.meeting_id,
                "sip": self.source_ip, "sport": self.source_port,
                "ssrc": self.ssrc, "ssrc_rtx": self.ssrc_rtx,
                "dip": self.dest_ip, "dport": self.dest_port
            }

        elif message_type == "set_quality":
            api_message = {
                "api": "set_quality",
                "mid": self.meeting_id,
                "sip": self.source_ip, "sport": self.source_port,
                "ssrc": self.ssrc,
                "dip": self.dest_ip, "dport": self.dest_port,
                "quality": self.quality.name
            }

        if not api_message:
            print(f"API message type {message_type} not found.")
        
        return api_message
    
    def display(self):
        print(f"        Receive stream: [ID {self.meeting_id}], [IP {self.source_ip}, port {self.source_port}]: " \
              + f"Dest. IP {self.dest_ip}, dest. port {self.dest_port}", f"SSRC {self.ssrc}, "\
              + f"rtx. SSRC {self.ssrc_rtx}, egress port {self.egress_port}, type {self.media_type}, quality {self.quality}")

########################################

class SFUSwitchAgent:

    ########################################

    def __init__(self, websocket, verbose=True):
        self.meetings     = {} # Key: meeting_id
        self.participants = {} # Key: {meeting_id, ip, port}
        self.snd_streams  = {} # Key: {meeting_id, sip, sport, ssrc}
        self.rcv_streams  = {} # Key: {meeting_id, sip, sport, dip, dport, ssrc}
        self.websocket    = websocket
        self.verbose      = verbose
    
    ########################################

    def printc(self, msg):
        if self.verbose:
            print(msg)

    ########################################
    
    def get_receive_streams(self):
        rcv_streams = []
        for rs in self.rcv_streams:
            rcv_streams.append(self.rcv_streams[rs].key())
            if self.rcv_streams[rs].ssrc_rtx != NO_RTX_SSRC:
                rcv_streams.append(self.rcv_streams[rs].key_rtx())
        return rcv_streams
    
    ########################################
    
    def add_meeting(self, meeting_id):
        if meeting_id in self.meetings:
            self.printc(f"Meeting with ID {meeting_id} already exists.")
            return
        self.meetings[meeting_id] = Meeting(meeting_id)
        # Local function call
        self.printc(f"  Local function call: add_meeting({meeting_id})")

    ########################################
    
    def add_participant(self, meeting_id, ip, port, veport):
        eport = ports.eport(veport)
        if meeting_id not in self.meetings:
            self.printc(f"Meeting with ID {meeting_id} does not exist.")
            return
        if (meeting_id, ip, port) in self.participants:
            self.printc(f"Participant with IP {ip} and port {port} already exists in meeting with ID {meeting_id}.")
            return
        self.participants[(meeting_id, ip, port)] = Participant(meeting_id, ip, port, eport)
        # Local function call
        self.printc(f"  Local function call: add_participant({meeting_id}, {ip}, {port}, {veport})")
    
    ########################################
    
    async def add_stream(self, meeting_id, sip, sport, ssrc, ssrc_rtx, eport, media_type):
        if meeting_id not in self.meetings:
            self.add_meeting(meeting_id)
        if (meeting_id, sip, sport) not in self.participants:
            self.add_participant(meeting_id, sip, sport, eport)
        if (meeting_id, sip, sport, ssrc) in self.snd_streams:
            self.printc(f"Stream with SSRC {ssrc} from participant with IP {sip} and port {sport} " \
                  + f"already exists in meeting with ID {meeting_id}.")
            return
        self.snd_streams[(meeting_id, sip, sport, ssrc)] = SendStream(
            meeting_id, sip, sport, ssrc, ssrc_rtx, media_type)
        # Local function call
        self.printc(f"  Local function call: add_stream({meeting_id}, {sip}, {sport}, {ssrc}, {ssrc_rtx}, {eport}, {media_type})")
        # Recompose the receive streams for this meeting
        await self.recompose_receive_streams()

    ########################################

    async def recompose_receive_streams(self):
        # Receive stream keys currently active
        active_rcv_stream_keys = []
        # Iterate over all send streams
        for (ss_mid, ss_ip, ss_port, ss_ssrc) in list(self.snd_streams):
            # Iterate over all participants
            for (p_mid, p_ip, p_port) in list(self.participants):
                if ss_mid == p_mid and (ss_ip != p_ip or ss_port != p_port):
                    # Build the receive stream key
                    active_rcv_stream_keys.append((ss_mid, ss_ip, ss_port, p_ip, p_port, ss_ssrc))
        # Check if any receive streams have become inactive and remove them
        for rcv_stream_key in list(self.rcv_streams):
            if rcv_stream_key not in active_rcv_stream_keys:
                # API call to the PRE: Remove data plane rules
                api_message = self.rcv_streams[rcv_stream_key].get_api_message("remove_stream")
                if api_message:
                    await self.websocket.send(json.dumps(api_message))
                    self.printc(f"  API call to PRE: {api_message}")
                # Remove the receive stream from local state
                del self.rcv_streams[rcv_stream_key]
        # Check if any new receive streams have become active and add them
        for rcv_stream_key in active_rcv_stream_keys:
            if rcv_stream_key not in self.rcv_streams:
                (mid, sip, sport, dip, dport, ssrc) = rcv_stream_key
                # Add the receive stream to local state
                self.rcv_streams[rcv_stream_key] = ReceiveStream(
                    mid, sip, sport, dip, dport, ssrc, self.snd_streams[(mid, sip, sport, ssrc)].ssrc_rtx,
                    self.participants[(mid, dip, dport)].egress_port, self.snd_streams[(mid, sip, sport, ssrc)].media_type)
                # API call to the PRE: Install data plane rules
                api_message = self.rcv_streams[rcv_stream_key].get_api_message("add_stream")
                if api_message:
                    await self.websocket.send(json.dumps(api_message))
                    self.printc(f"  API call to PRE: {api_message}")

    ########################################
    
    async def remove_stream(self, meeting_id, sip, sport, ssrc):
        if (meeting_id, sip, sport, ssrc) not in self.snd_streams:
            self.printc(f"Send stream with meeting ID {meeting_id}, IP {sip}, port {sport} and SSRC {ssrc} does not exist.")
            return
        # Remove the send stream
        del self.snd_streams[(meeting_id, sip, sport, ssrc)]
        # Local function call
        self.printc(f"  Local function call: remove_stream({meeting_id}, {sip}, {sport}, {ssrc})")
        # Check if the participant has any remaining send streams; remove participant if not
        is_participant_active = False
        for (ss_mid, ss_sip, ss_sport, _) in list(self.snd_streams):
            if ss_mid == meeting_id and ss_sip == sip and ss_sport == sport:
                is_participant_active = True
                break
        if not is_participant_active:
            self.remove_participant(meeting_id, sip, sport)
        # Check if the meeting has any remaining participants; remove meeting if not
        is_meeting_active = False
        for (p_mid, _, _) in list(self.participants):
            if p_mid == meeting_id:
                is_meeting_active = True
                break
        if not is_meeting_active:
            self.remove_meeting(meeting_id)
        # Recompose the receive streams for this meeting
        await self.recompose_receive_streams()
    
    ########################################
    
    def remove_participant(self, meeting_id, sip, sport):
        if (meeting_id, sip, sport) not in self.participants:
            self.printc(f"Participant with meeting ID {meeting_id}, IP {sip} and port {sport} does not exist.")
            return
        # Remove the participant
        del self.participants[meeting_id, sip, sport]
        # Local function call
        self.printc(f"  Local function call: remove_participant({meeting_id}, {sip}, {sport})")
    
    ########################################
    
    def remove_meeting(self, meeting_id):
        if meeting_id not in self.meetings:
            self.printc(f"Meeting with meeting ID {meeting_id} does not exist.")
            return
        # Remove the meeting
        del self.meetings[meeting_id]
        # Local function call
        self.printc(f"  Local function call: end_meeting({meeting_id})")
    
    ########################################

    async def set_quality(self, meeting_id, sip, sport, ssrc, dip, dport, quality):
        # API call to the PRE: Disable the enhancement layer
        rcv_stream_key = (meeting_id, sip, sport, dip, dport, ssrc)
        if rcv_stream_key not in self.rcv_streams:
            print(f"Receive stream {rcv_stream_key} not found.")
        else:
            print(quality, quality.name, quality.value)
            self.rcv_streams[rcv_stream_key].quality = quality
            api_message = self.rcv_streams[rcv_stream_key].get_api_message("set_quality")
            if api_message:
                await self.websocket.send(json.dumps(api_message))
                self.printc(f"  API call to PRE: {api_message}")

    ########################################
        
    def display_meetings(self):
        self.printc("SFU meetings summary:")
        if len(self.meetings) == 0:
            print("  No active meetings.")
        for m_mid in self.meetings:
            self.meetings[m_mid].display()
            for (p_mid, p_ip, p_port) in self.participants:
                if p_mid == m_mid:
                    self.participants[(p_mid, p_ip, p_port)].display()
                    for (ss_mid, ss_ip, ss_port, ss_ssrc) in self.snd_streams:
                        if ss_mid == m_mid and ss_ip == p_ip and ss_port == p_port:
                            self.snd_streams[(ss_mid, ss_ip, ss_port, ss_ssrc)].display()
                            for (rs_mid, rs_sip, rs_sport, rs_dip, rs_dport, rs_ssrc) in self.rcv_streams:
                                if rs_mid == ss_mid and rs_sip == ss_ip and rs_sport == ss_port and rs_ssrc == ss_ssrc:
                                    self.rcv_streams[(rs_mid, rs_sip, rs_sport, rs_dip, rs_dport, rs_ssrc)].display()
    
    ########################################

########################################

async def sfu_switch_agent(websocket, test_mode=False):
    
    print("Connected to the Tofino PRE via WebSocket.")

    # Create an instance of the SFU switch agent and pass the websocket object to it
    sfuSwitchAgent = SFUSwitchAgent(websocket)

    if test_mode:

        ## Orchestrate a test meeting

        # Hardcoded numbers
        ## Meeting ID
        M_ID = 0
        ## IP, port
        P1_IP, P2_IP, P3_IP = "10.0.211.2", "10.0.211.2", "10.0.211.2"
        P1_PORT, P2_PORT, P3_PORT = 1111, 2222, 3333
        P1_EPORT, P2_EPORT, P3_EPORT = "veth4", "veth6", "veth6" # veth4 is Port 2 on Tofino1, veth6 is Port 3 on Tofino1
        ## P1 SSRCs
        P1_V_SSRC, P1_V_SSRC_RTX, P1_A_SSRC, P1_S_SSRC = 110, 111, 120, 130
        ## P2 SSRCs
        P2_V_SSRC, P2_V_SSRC_RTX, P2_A_SSRC = 210, 211, 220
        ## P3 SSRCs
        P3_V_SSRC, P3_V_SSRC_RTX, P3_A_SSRC = 310, 311, 320

        # (1) Participants P1, P2, P3 join meeting M one by one with their video and audio on
        print("Event(s): Participants P1, P2, P3 have joined meeting M one by one with their video and audio on")
        await sfuSwitchAgent.add_stream(M_ID, P1_IP, P1_PORT, P1_V_SSRC, P1_V_SSRC_RTX, P1_EPORT, Media.video)
        # await sfuSwitchAgent.add_stream(M_ID, P1_IP, P1_PORT, P1_A_SSRC, NO_RTX_SSRC, P1_EPORT, Media.audio)
        await sfuSwitchAgent.add_stream(M_ID, P2_IP, P2_PORT, P2_V_SSRC, P2_V_SSRC_RTX, P2_EPORT, Media.video)
        # await sfuSwitchAgent.add_stream(M_ID, P2_IP, P2_PORT, P2_A_SSRC, NO_RTX_SSRC, P2_EPORT, Media.audio)
        await sfuSwitchAgent.add_stream(M_ID, P3_IP, P3_PORT, P3_V_SSRC, P3_V_SSRC_RTX, P3_EPORT, Media.video)
        # await sfuSwitchAgent.add_stream(M_ID, P3_IP, P3_PORT, P3_A_SSRC, NO_RTX_SSRC, P3_EPORT, Media.audio)
        sfuSwitchAgent.display_meetings()

        # # (1.1) Disable the enhancement layer for P1's video stream to P3
        # print("Event(s): Disable the enhancement layer for P1's video stream to P3")
        # await sfuSwitchAgent.set_quality(M_ID, P1_IP, P1_PORT, P1_V_SSRC, P3_IP, P3_PORT, Quality.base)

        # # (1.2) Reenable the enhancement layer for P1's video stream to P3
        # print("Event(s): Reenable the enhancement layer for P1's video stream to P3")
        # await sfuSwitchAgent.reenable_av1_enhancement_layer(M_ID, P1_IP, P1_PORT, P1_V_SSRC, P3_IP, P3_PORT)

        # # (2) P2 and P3 mute their audio
        # print("\nEvent(s): Participants P2, P3 mute audio")
        # await sfuSwitchAgent.remove_stream(M_ID, P2_IP, P2_PORT, P2_A_SSRC)
        # await sfuSwitchAgent.remove_stream(M_ID, P3_IP, P3_PORT, P3_A_SSRC)
        # sfuSwitchAgent.display_meetings()

        # # (3) P1 starts screen-sharing
        # print("\nEvent(s): Participant P1 starts screensharing")
        # await sfuSwitchAgent.add_stream(M_ID, P1_IP, P1_PORT, P1_S_SSRC, NO_RTX_SSRC, P1_EPORT, Media.screenshare)
        # sfuSwitchAgent.display_meetings()

        # # (4) P2 mutes video
        # print("\nEvent(s): Participant P2 mutes video, becoming inactive")
        # await sfuSwitchAgent.remove_stream(M_ID, P2_IP, P2_PORT, P2_V_SSRC)
        # sfuSwitchAgent.display_meetings()

        # # (5) P3 unmutes audio
        # print("\nEvent(s): Participant P3 unmutes audio")
        # await sfuSwitchAgent.add_stream(M_ID, P3_IP, P3_PORT, P3_A_SSRC, NO_RTX_SSRC, P3_EPORT, Media.audio)
        # sfuSwitchAgent.display_meetings()

        # # (6) P3 mutes audio again
        # print("\nEvent(s): Participant P3 mutes audio")
        # await sfuSwitchAgent.remove_stream(M_ID, P3_IP, P3_PORT, P3_A_SSRC)
        # sfuSwitchAgent.display_meetings()

        # # (7) P1 stops screen-sharing
        # print("\nEvent(s): Participant P1 stops screensharing")
        # await sfuSwitchAgent.remove_stream(M_ID, P1_IP, P1_PORT, P1_S_SSRC)
        # sfuSwitchAgent.display_meetings()

        # # (8) P2 unmutes video and audio
        # print("\nEvent(s): Participant P2 unmutes video and audio, becoming active again")
        # await sfuSwitchAgent.add_stream(M_ID, P2_IP, P2_PORT, P2_V_SSRC, P2_V_SSRC_RTX, P2_EPORT, Media.video)
        # await sfuSwitchAgent.add_stream(M_ID, P2_IP, P2_PORT, P2_A_SSRC, NO_RTX_SSRC, P2_EPORT, Media.audio)
        # sfuSwitchAgent.display_meetings()

        # # (9) P2 leaves meeting M
        # print("\nEvent(s): Participant P2 leaves the meeting M, resulting in her video and audio streams being removed")
        # await sfuSwitchAgent.remove_stream(M_ID, P2_IP, P2_PORT, P2_V_SSRC)
        # await sfuSwitchAgent.remove_stream(M_ID, P2_IP, P2_PORT, P2_A_SSRC)
        # sfuSwitchAgent.display_meetings()

        # # (10) P3 unmutes audio
        # print("\nEvent(s): Participant P3 unmutes audio")
        # await sfuSwitchAgent.add_stream(M_ID, P3_IP, P3_PORT, P3_A_SSRC, NO_RTX_SSRC, P3_EPORT, Media.audio)
        # sfuSwitchAgent.display_meetings()

        # # (11) Meeting M ends
        # print("\nEvent(s): Meeting M ends, causing removal of all the remaining streams")
        # await sfuSwitchAgent.remove_stream(M_ID, P1_IP, P1_PORT, P1_V_SSRC)
        # await sfuSwitchAgent.remove_stream(M_ID, P1_IP, P1_PORT, P1_A_SSRC)
        # await sfuSwitchAgent.remove_stream(M_ID, P3_IP, P3_PORT, P3_V_SSRC)
        # await sfuSwitchAgent.remove_stream(M_ID, P3_IP, P3_PORT, P3_A_SSRC)
        # sfuSwitchAgent.display_meetings()

    os._exit(0)

########################################

async def main():

    # Check if testing is enabled
    parser = argparse.ArgumentParser(description="Allows testing mode to be enabled.")
    parser.add_argument('--test', action='store_true', help="Enable testing.")
    args = parser.parse_args()

    # Instantiate a WebSocket server
    websocketServer = websockets.serve(
        lambda ws, request_headers=None, **kwargs: sfu_switch_agent(ws, args.test),
        host="localhost",
        port=8765,
    )

    # Start the WebSocket server
    async with websocketServer:
        print("SFU switch agent started.")
        await asyncio.Future()

########################################

if __name__ == "__main__":
    asyncio.run(main())

########################################
