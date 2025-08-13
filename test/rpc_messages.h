
#include <string>

namespace test {

    static std::string hello_rpc_json = R"({"type":"hello"})";

    static std::string sw_hello_switch_rpc_json = R"({"data":{"switch_id":1},"type":"hello"})";

    static std::string sw_add_participant_rpc_json = R"({"data":{"ip":"1.2.3.4","participant_id":2,"session_id":1},"type":"add_participant"})";

    static std::string sw_add_stream_rpc_json = R"({"data":{"direction":"sendonly","ice_pwd":"dsfjsdksddfkdmfdk03","ice_ufrag":"fksl3","ip":"1.2.3.4","main_ssrc":24900303,"media_type":"video","participant_id":2,"port":2322,"rtcp_rtx_ssrc":24829929,"rtcp_ssrc":52902399,"rtx_ssrc":329439892,"session_id":1},"type":"add_stream"})";

    static std::string cl_hello_session_rpc_json = R"({"data":{"session_id":1234},"type":"hello"})";

    static std::string cl_hello_participant_rpc_json = R"({"data":{"participant_id":429},"type":"hello"})";


    static std::string cl_offer_rpc_json = R"({"data":{"participant_id":5,"sdp":"TEST_SDP","type":"offer"},"type":"offer"})";

    static std::string cl_answer_rpc_json = R"({"data":{"participant_id":5,"sdp":"TEST_SDP","type":"answer"},"type":"answer"})";
}
