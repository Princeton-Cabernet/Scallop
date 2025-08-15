#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>
#include <vector>
#include <string>
#include <chrono>
#include <cmath>
#include <map>

using namespace std;

/******************** Struct declarations ********************/

struct PerfComboParams {
    int tf_version;
    int num_participants;
    int num_senders;
    string rep_mode;
    string sr_mode;
    int num_quality_high;
    int num_quality_mid;
    int num_quality_low;
    float loss_rate;
    float retx_overhead;
    string params_str = "None";

    void populate_str() {
        params_str = "v" + to_string(tf_version) + ":" + to_string(num_participants) + ":" + to_string(num_senders) + ":"
                     + rep_mode + ":" + sr_mode + ":"
                     + to_string(num_quality_high) + ":" + to_string(num_quality_mid) + ":" + to_string(num_quality_low)
                     + ":" + to_string(int(loss_rate)) + ":" + to_string(int(retx_overhead));
    }
};

struct PerfCombo {
    PerfComboParams params;
    int sw = -1;
    int hw_bw = -1;
    int hw_rep = -1;
    int hw_nra = -1;
    int hw_rar = -1;
    int hw_rasr = -1;
    int hw_mem = -1;
    int hw_slm = -1;
    int hw_slr = -1;
    int hw = -1;
    float scale = -1;
    string combo_str = params.params_str;

    void populate_str() {
        params.populate_str();
        combo_str = params.params_str + ":" + to_string(sw) + ":" + to_string(hw_bw) + ":" + to_string(hw_rep)
                    + ":" + to_string(hw_mem) + ":" + to_string(hw);
    }

    void print() {
        cout << params.tf_version << " " << params.num_participants << " " << params.num_senders
             << " " << params.rep_mode << " " << params.sr_mode
             << " " << params.num_quality_high << " " << params.num_quality_mid << " " << params.num_quality_low
             << " " << params.loss_rate << " " << params.retx_overhead
             << " " << sw << " " << hw_bw << " " << hw_rep << " " << hw_mem << " " << hw << " " << scale << endl;
    }
};

struct PerfResource {
    bool initialized = false;
    int min = -1;
    int max = -1;
    PerfComboParams min_params;
    PerfComboParams max_params;
};

struct PerfScale {
    bool initialized = false;
    float min = -1;
    float max = -1;
    PerfComboParams min_params;
    PerfComboParams max_params;
};

struct PerfParticipantCount {
    PerfResource software;
    PerfResource hardware_bw;
    PerfResource hardware_rep;
    PerfResource hardware_nra;
    PerfResource hardware_rar;
    PerfResource hardware_rasr;
    PerfResource hardware_mem;
    PerfResource hardware_slm;
    PerfResource hardware_slr;
    PerfResource hardware;
    PerfScale non_rate_adapted;
    PerfScale rate_adapted;
    PerfScale all;
    long long unsigned int combo_count = 0;
};

struct PerfResults {
    map<int, PerfParticipantCount> tofino1;
    map<int, PerfParticipantCount> tofino2;
};

/******************** Function definitions ********************/

string elapsed_time(const std::chrono::time_point<std::chrono::high_resolution_clock>& start,
        const std::chrono::time_point<std::chrono::high_resolution_clock>& end = std::chrono::high_resolution_clock::now()) {
    using namespace std::chrono;
    auto total_seconds = duration_cast<seconds>(end - start).count();
    int days = static_cast<int>(total_seconds / 86400);
    total_seconds %= 86400;
    int hours = static_cast<int>(total_seconds / 3600);
    total_seconds %= 3600;
    int minutes = static_cast<int>(total_seconds / 60);
    total_seconds %= 60;
    int seconds = static_cast<int>(total_seconds);
    std::ostringstream oss;
    if (days > 0)    oss << days << "d";
    if (days > 0 && hours > 0) oss << " ";
    if (hours > 0)   oss << hours << "h";
    if ((days > 0 || hours > 0) && minutes > 0) oss << " ";
    if (minutes > 0) oss << minutes << "m";
    if ((days > 0 || hours > 0 || minutes > 0) && seconds > 0) oss << " ";
    if (seconds > 0 || oss.str().empty()) oss << seconds << "s";
    return oss.str();
}

float get_tofino_bandwidth(int tf_version=2) {
    // Get bandwidth of the Tofino version
    float tf_bandwidth = -1;
    if (tf_version == 1)
        // Tofino 1 has 6.4 Tbps bandwidth
        tf_bandwidth = 6.4; // Tbps
    else if (tf_version == 2)
        // Tofino 2 has 12.8 Tbps bandwidth
        tf_bandwidth = 12.8; // Tbps
    else {
        cout << "Tofino version is invalid: " << tf_version << endl;
        exit(1);
    }
    return tf_bandwidth;
}

map<string, int> get_tofino_memory_limit(int tf_version=2) {
    // Get memory limit of the Tofino version
    // The memory limit is found by compiling the Tofino program -- with different register array sizes -- on the hardware switch
    int MAX_MEMORY_PER_STAGE = 143500;
    map<string, int> memory_limits = {{"S-LM", -1}, {"S-LR", -1}};
    if (tf_version == 1) {
        memory_limits["S-LM"] = MAX_MEMORY_PER_STAGE * 8;
        memory_limits["S-LR"] = MAX_MEMORY_PER_STAGE;
    }   
    else if (tf_version == 2) {
        memory_limits["S-LM"] = MAX_MEMORY_PER_STAGE * 16;
        memory_limits["S-LR"] = MAX_MEMORY_PER_STAGE * 3;
    }
    else {
        cout << "Tofino version is invalid: " << tf_version << endl;
        exit(1);
    }
    return memory_limits;
}

PerfCombo compute_perf_for_combo(PerfComboParams params) {
    // Compute the performance of a given combination of parameters
    PerfCombo perf_combo = {
        params,
        -1, // sw
        -1, // hw_bw
        -1, // hw_rep
        -1, // hw_mem
        -1, // hw
        -1  // scale
    };
    float tf_bandwidth = get_tofino_bandwidth(perf_combo.params.tf_version);
    map<string, int> memory_limits = get_tofino_memory_limit(perf_combo.params.tf_version);
    int MAX_REPLICATION_TREES = 64000; // 64K is the max. number of replication trees on the Tofino
    float bw_per_stream_high = 3.0; // Mbps
    float bw_per_stream_mid  = 1.5; // Mbps
    float bw_per_stream_low  = 1.0; // Mbps

    // Software: maximum work
    float sw_bandwidth = 32 * (8 * 10 * 10) * bw_per_stream_high; // 32 cores

    // Work requirement per meeting
    // Work requirement of the receivers
    float sw_bw_req = (bw_per_stream_high * perf_combo.params.num_quality_high)
                        + (bw_per_stream_mid * perf_combo.params.num_quality_mid)
                        + (bw_per_stream_low * perf_combo.params.num_quality_low);
    // Work requirement of the senders
    if (perf_combo.params.num_quality_high > 0)
        sw_bw_req += bw_per_stream_high * perf_combo.params.num_senders;
    else if (perf_combo.params.num_quality_mid > 0)
        sw_bw_req += bw_per_stream_mid * perf_combo.params.num_senders;
    else
        sw_bw_req += bw_per_stream_low * perf_combo.params.num_senders;
    // Account for retransmission overhead
    sw_bw_req += (perf_combo.params.retx_overhead/100) * sw_bw_req;

    // Software: No. of meetings
    perf_combo.sw = floor(sw_bandwidth / sw_bw_req);

    // Hardware
    // Hardware bandwidth: requirement per meeting
    float hw_bw_req = sw_bw_req;
    // Hardware: number of meetings
    perf_combo.hw_bw = floor((tf_bandwidth * pow(10, 6)) / hw_bw_req);

    // Hardware: Replication
    if (perf_combo.params.num_participants > 2) {
        if (perf_combo.params.rep_mode == "NRA") {
            perf_combo.hw_rep = MAX_REPLICATION_TREES * 2;
            perf_combo.hw_nra = MAX_REPLICATION_TREES * 2;
        }
        else if (perf_combo.params.rep_mode == "RA-R") {
            if ((perf_combo.params.num_quality_high > 0) + (perf_combo.params.num_quality_mid > 0) + (perf_combo.params.num_quality_low > 0) == 2) {
                perf_combo.hw_rep = MAX_REPLICATION_TREES;
                perf_combo.hw_rar = MAX_REPLICATION_TREES;
            } else if ((perf_combo.params.num_quality_high > 0) + (perf_combo.params.num_quality_mid > 0) + (perf_combo.params.num_quality_low > 0) == 3) {
                perf_combo.hw_rep = floor(MAX_REPLICATION_TREES * 2.0 / 3);
                perf_combo.hw_rar = floor(MAX_REPLICATION_TREES * 2.0 / 3);
            } else {
                cout << "Replication mode quality distribution is invalid: " << perf_combo.params.params_str << endl;
                exit(1);
            }
        }
        else if (perf_combo.params.rep_mode == "RA-SR") {
            if ((perf_combo.params.num_quality_high > 0) + (perf_combo.params.num_quality_mid > 0) + (perf_combo.params.num_quality_low > 0) == 2) {
                perf_combo.hw_rep  = floor(MAX_REPLICATION_TREES * 2.0 / (2 * perf_combo.params.num_senders));
                perf_combo.hw_rasr = floor(MAX_REPLICATION_TREES * 2.0 / (2 * perf_combo.params.num_senders));
            } else if ((perf_combo.params.num_quality_high > 0) + (perf_combo.params.num_quality_mid > 0) + (perf_combo.params.num_quality_low > 0) == 3) {
                perf_combo.hw_rep  = floor(MAX_REPLICATION_TREES * 2.0 / (3 * perf_combo.params.num_senders));
                perf_combo.hw_rasr = floor(MAX_REPLICATION_TREES * 2.0 / (3 * perf_combo.params.num_senders));
            }
        }
        else {
            cout << "Replication mode is invalid: " << perf_combo.params.params_str << endl;
            exit(1);
        }
    }

    // Hardware: Memory
    if (perf_combo.params.num_participants > 2) {
        if (perf_combo.params.sr_mode == "S-LM") {
            perf_combo.hw_mem = floor(memory_limits["S-LM"] / (perf_combo.params.num_senders * (perf_combo.params.num_participants - 1)));
            perf_combo.hw_slm = floor(memory_limits["S-LM"] / (perf_combo.params.num_senders * (perf_combo.params.num_participants - 1)));
        } else if (perf_combo.params.sr_mode == "S-LR") {
            perf_combo.hw_mem = floor(memory_limits["S-LR"] / (perf_combo.params.num_senders * (perf_combo.params.num_participants - 1)));
            perf_combo.hw_slr = floor(memory_limits["S-LR"] / (perf_combo.params.num_senders * (perf_combo.params.num_participants - 1)));
        } else if (perf_combo.params.sr_mode == "None") {
            if (!(perf_combo.params.rep_mode == "NRA")) {
                cout << "Sequence number rewriting mode is invalid: " << perf_combo.params.params_str << endl;
                exit(1);
            }
        }
        else {
            cout << "Sequence number rewriting mode is invalid: " << perf_combo.params.params_str << endl;
            exit(1);
        }
    }

    // Hardware: Bottleneck
    perf_combo.hw = perf_combo.hw_bw;
    if ((perf_combo.hw_rep > 0) && (perf_combo.hw_rep < perf_combo.hw_bw))
        perf_combo.hw = perf_combo.hw_rep;
    if ((perf_combo.hw_mem > 0) && (perf_combo.hw_mem < perf_combo.hw))
        perf_combo.hw = perf_combo.hw_mem;

    // Scale improvement
    perf_combo.scale = perf_combo.hw * 1.0 / perf_combo.sw;

    // if (params.tf_version == 2)
    //     perf_combo.print();

    // Testing
    assert(perf_combo.sw <= 38400 && perf_combo.sw >= 2);
    // if (params.tf_version == 1) {
    //     assert(perf_combo.hw_bw <= 64000 && perf_combo.hw_bw >= 2);
    // } else if (params.tf_version == 2) {
    //     assert(perf_combo.hw_bw <= 128000 && perf_combo.hw_bw >= 2);
    // }

    return perf_combo;
}

void check_and_initialize(PerfParticipantCount& perf_participant_count, PerfCombo& perf_combo) {
    // Check and initialize the performance results for a given number of participants
    if (!perf_participant_count.software.initialized) {
        perf_participant_count.software.min = perf_combo.sw;
        perf_participant_count.software.max = perf_combo.sw;
        perf_combo.params.populate_str();
        perf_participant_count.software.min_params = perf_combo.params;
        perf_participant_count.software.max_params = perf_combo.params;
        perf_participant_count.software.initialized = true;
    }
    if (!perf_participant_count.hardware_bw.initialized) {
        perf_participant_count.hardware_bw.min = perf_combo.hw_bw;
        perf_participant_count.hardware_bw.max = perf_combo.hw_bw;
        perf_combo.params.populate_str();
        perf_participant_count.hardware_bw.min_params = perf_combo.params;
        perf_participant_count.hardware_bw.max_params = perf_combo.params;
        perf_participant_count.hardware_bw.initialized = true;
    }
    if (!perf_participant_count.hardware_rep.initialized && perf_combo.hw_rep > 0) {
        perf_participant_count.hardware_rep.min = perf_combo.hw_rep;
        perf_participant_count.hardware_rep.max = perf_combo.hw_rep;
        perf_combo.params.populate_str();
        perf_participant_count.hardware_rep.min_params = perf_combo.params;
        perf_participant_count.hardware_rep.max_params = perf_combo.params;
        perf_participant_count.hardware_rep.initialized = true;
    }
    if (!perf_participant_count.hardware_nra.initialized && perf_combo.hw_nra > 0) {
        perf_participant_count.hardware_nra.min = perf_combo.hw_nra;
        perf_participant_count.hardware_nra.max = perf_combo.hw_nra;
        perf_combo.params.populate_str();
        perf_participant_count.hardware_nra.min_params = perf_combo.params;
        perf_participant_count.hardware_nra.max_params = perf_combo.params;
        perf_participant_count.hardware_nra.initialized = true;
    }
    if (!perf_participant_count.hardware_rar.initialized && perf_combo.hw_rar > 0) {
        perf_participant_count.hardware_rar.min = perf_combo.hw_rar;
        perf_participant_count.hardware_rar.max = perf_combo.hw_rar;
        perf_combo.params.populate_str();
        perf_participant_count.hardware_rar.min_params = perf_combo.params;
        perf_participant_count.hardware_rar.max_params = perf_combo.params;
        perf_participant_count.hardware_rar.initialized = true;
    }
    if (!perf_participant_count.hardware_rasr.initialized && perf_combo.hw_rasr > 0) {
        perf_participant_count.hardware_rasr.min = perf_combo.hw_rasr;
        perf_participant_count.hardware_rasr.max = perf_combo.hw_rasr;
        perf_combo.params.populate_str();
        perf_participant_count.hardware_rasr.min_params = perf_combo.params;
        perf_participant_count.hardware_rasr.max_params = perf_combo.params;
        perf_participant_count.hardware_rasr.initialized = true;
    }
    if (!perf_participant_count.hardware_mem.initialized && perf_combo.hw_mem > 0) {
        perf_participant_count.hardware_mem.min = perf_combo.hw_mem;
        perf_participant_count.hardware_mem.max = perf_combo.hw_mem;
        perf_combo.params.populate_str();
        perf_participant_count.hardware_mem.min_params = perf_combo.params;
        perf_participant_count.hardware_mem.max_params = perf_combo.params;
        perf_participant_count.hardware_mem.initialized = true;
    }
    if (!perf_participant_count.hardware_slm.initialized && perf_combo.hw_slm > 0) {
        perf_participant_count.hardware_slm.min = perf_combo.hw_slm;
        perf_participant_count.hardware_slm.max = perf_combo.hw_slm;
        perf_combo.params.populate_str();
        perf_participant_count.hardware_slm.min_params = perf_combo.params;
        perf_participant_count.hardware_slm.max_params = perf_combo.params;
        perf_participant_count.hardware_slm.initialized = true;
    }
    if (!perf_participant_count.hardware_slr.initialized && perf_combo.hw_slr > 0) {
        perf_participant_count.hardware_slr.min = perf_combo.hw_slr;
        perf_participant_count.hardware_slr.max = perf_combo.hw_slr;
        perf_combo.params.populate_str();
        perf_participant_count.hardware_slr.min_params = perf_combo.params;
        perf_participant_count.hardware_slr.max_params = perf_combo.params;
        perf_participant_count.hardware_slr.initialized = true;
    }
    if (!perf_participant_count.hardware.initialized) {
        perf_participant_count.hardware.min = perf_combo.hw;
        perf_participant_count.hardware.max = perf_combo.hw;
        perf_combo.params.populate_str();
        perf_participant_count.hardware.min_params = perf_combo.params;
        perf_participant_count.hardware.max_params = perf_combo.params;
        perf_participant_count.hardware.initialized = true;
    }
    if (!perf_participant_count.all.initialized) {
        perf_participant_count.all.min = perf_combo.scale;
        perf_participant_count.all.max = perf_combo.scale;
        perf_combo.params.populate_str();
        perf_participant_count.all.min_params = perf_combo.params;
        perf_participant_count.all.max_params = perf_combo.params;
        perf_participant_count.all.initialized = true;
    }
    if (!perf_participant_count.non_rate_adapted.initialized && perf_combo.params.rep_mode == "NRA") {
        perf_participant_count.non_rate_adapted.min = perf_combo.scale;
        perf_participant_count.non_rate_adapted.max = perf_combo.scale;
        perf_combo.params.populate_str();
        perf_participant_count.non_rate_adapted.min_params = perf_combo.params;
        perf_participant_count.non_rate_adapted.max_params = perf_combo.params;
        perf_participant_count.non_rate_adapted.initialized = true;
    }
    if (!perf_participant_count.rate_adapted.initialized && (perf_combo.params.rep_mode == "RA-R" || perf_combo.params.rep_mode == "RA-SR")) {
        perf_participant_count.rate_adapted.min = perf_combo.scale;
        perf_participant_count.rate_adapted.max = perf_combo.scale;
        perf_combo.params.populate_str();
        perf_participant_count.rate_adapted.min_params = perf_combo.params;
        perf_participant_count.rate_adapted.max_params = perf_combo.params;
        perf_participant_count.rate_adapted.initialized = true;
    }
}

void check_and_update(PerfParticipantCount& perf_participant_count, PerfCombo& perf_combo) {
    // Check and update the min/max performance results for a given number of participants
    if (perf_participant_count.software.initialized) {
        if (perf_combo.sw < perf_participant_count.software.min) {
            perf_participant_count.software.min = perf_combo.sw;
            perf_combo.params.populate_str();
            perf_participant_count.software.min_params = perf_combo.params;
        }
        if (perf_combo.sw > perf_participant_count.software.max) {
            perf_participant_count.software.max = perf_combo.sw;
            perf_combo.params.populate_str();
            perf_participant_count.software.max_params = perf_combo.params;
        }
    }
    if (perf_participant_count.hardware_bw.initialized) {
        if (perf_combo.hw_bw < perf_participant_count.hardware_bw.min) {
            perf_participant_count.hardware_bw.min = perf_combo.hw_bw;
            perf_combo.params.populate_str();
            perf_participant_count.hardware_bw.min_params = perf_combo.params;
        }
        if (perf_combo.hw_bw > perf_participant_count.hardware_bw.max) {
            perf_participant_count.hardware_bw.max = perf_combo.hw_bw;
            perf_combo.params.populate_str();
            perf_participant_count.hardware_bw.max_params = perf_combo.params;
        }
    }
    if (perf_participant_count.hardware_rep.initialized && perf_combo.hw_rep > 0) {
        if (perf_combo.hw_rep < perf_participant_count.hardware_rep.min) {
            perf_participant_count.hardware_rep.min = perf_combo.hw_rep;
            perf_combo.params.populate_str();
            perf_participant_count.hardware_rep.min_params = perf_combo.params;
        }
        if (perf_combo.hw_rep > perf_participant_count.hardware_rep.max) {
            perf_participant_count.hardware_rep.max = perf_combo.hw_rep;
            perf_combo.params.populate_str();
            perf_participant_count.hardware_rep.max_params = perf_combo.params;
        }
    }
    if (perf_participant_count.hardware_nra.initialized && perf_combo.hw_nra > 0) {
        if (perf_combo.hw_nra < perf_participant_count.hardware_nra.min) {
            perf_participant_count.hardware_nra.min = perf_combo.hw_nra;
            perf_combo.params.populate_str();
            perf_participant_count.hardware_nra.min_params = perf_combo.params;
        }
        if (perf_combo.hw_nra > perf_participant_count.hardware_nra.max) {
            perf_participant_count.hardware_nra.max = perf_combo.hw_nra;
            perf_combo.params.populate_str();
            perf_participant_count.hardware_nra.max_params = perf_combo.params;
        }
    }
    if (perf_participant_count.hardware_rar.initialized && perf_combo.hw_rar > 0) {
        if (perf_combo.hw_rar < perf_participant_count.hardware_rar.min) {
            perf_participant_count.hardware_rar.min = perf_combo.hw_rar;
            perf_combo.params.populate_str();
            perf_participant_count.hardware_rar.min_params = perf_combo.params;
        }
        if (perf_combo.hw_rar > perf_participant_count.hardware_rar.max) {
            perf_participant_count.hardware_rar.max = perf_combo.hw_rar;
            perf_combo.params.populate_str();
            perf_participant_count.hardware_rar.max_params = perf_combo.params;
        }
    }
    if (perf_participant_count.hardware_rasr.initialized && perf_combo.hw_rasr > 0) {
        if (perf_combo.hw_rasr < perf_participant_count.hardware_rasr.min) {
            perf_participant_count.hardware_rasr.min = perf_combo.hw_rasr;
            perf_combo.params.populate_str();
            perf_participant_count.hardware_rasr.min_params = perf_combo.params;
        }
        if (perf_combo.hw_rasr > perf_participant_count.hardware_rasr.max) {
            perf_participant_count.hardware_rasr.max = perf_combo.hw_rasr;
            perf_combo.params.populate_str();
            perf_participant_count.hardware_rasr.max_params = perf_combo.params;
        }
    }
    if (perf_participant_count.hardware_mem.initialized && perf_combo.hw_mem > 0) {
        if (perf_combo.hw_mem < perf_participant_count.hardware_mem.min) {
            perf_participant_count.hardware_mem.min = perf_combo.hw_mem;
            perf_combo.params.populate_str();
            perf_participant_count.hardware_mem.min_params = perf_combo.params;
        }
        if (perf_combo.hw_mem > perf_participant_count.hardware_mem.max) {
            perf_participant_count.hardware_mem.max = perf_combo.hw_mem;
            perf_combo.params.populate_str();
            perf_participant_count.hardware_mem.max_params = perf_combo.params;
        }
    }
    if (perf_participant_count.hardware_slm.initialized && perf_combo.hw_slm > 0) {
        if (perf_combo.hw_slm < perf_participant_count.hardware_slm.min) {
            perf_participant_count.hardware_slm.min = perf_combo.hw_slm;
            perf_combo.params.populate_str();
            perf_participant_count.hardware_slm.min_params = perf_combo.params;
        }
        if (perf_combo.hw_slm > perf_participant_count.hardware_slm.max) {
            perf_participant_count.hardware_slm.max = perf_combo.hw_slm;
            perf_combo.params.populate_str();
            perf_participant_count.hardware_slm.max_params = perf_combo.params;
        }
    }
    if (perf_participant_count.hardware_slr.initialized && perf_combo.hw_slr > 0) {
        if (perf_combo.hw_slr < perf_participant_count.hardware_slr.min) {
            perf_participant_count.hardware_slr.min = perf_combo.hw_slr;
            perf_combo.params.populate_str();
            perf_participant_count.hardware_slr.min_params = perf_combo.params;
        }
        if (perf_combo.hw_slr > perf_participant_count.hardware_slr.max) {
            perf_participant_count.hardware_slr.max = perf_combo.hw_slr;
            perf_combo.params.populate_str();
            perf_participant_count.hardware_slr.max_params = perf_combo.params;
        }
    }
    if (perf_participant_count.hardware.initialized) {
        if (perf_combo.hw < perf_participant_count.hardware.min) {
            perf_participant_count.hardware.min = perf_combo.hw;
            perf_combo.params.populate_str();
            perf_participant_count.hardware.min_params = perf_combo.params;
        }
        if (perf_combo.hw > perf_participant_count.hardware.max) {
            perf_participant_count.hardware.max = perf_combo.hw;
            perf_combo.params.populate_str();
            perf_participant_count.hardware.max_params = perf_combo.params;
        }
    }
    if (perf_participant_count.all.initialized) {
        if (perf_combo.scale < perf_participant_count.all.min) {
            perf_participant_count.all.min = perf_combo.scale;
            perf_combo.params.populate_str();
            perf_participant_count.all.min_params = perf_combo.params;
        }
        if (perf_combo.scale > perf_participant_count.all.max) {
            perf_participant_count.all.max = perf_combo.scale;
            perf_combo.params.populate_str();
            perf_participant_count.all.max_params = perf_combo.params;
        }
    }
    if (perf_participant_count.non_rate_adapted.initialized && perf_combo.params.rep_mode == "NRA") {
        if (perf_combo.scale < perf_participant_count.non_rate_adapted.min) {
            perf_participant_count.non_rate_adapted.min = perf_combo.scale;
            perf_combo.params.populate_str();
            perf_participant_count.non_rate_adapted.min_params = perf_combo.params;
        }
        if (perf_combo.scale > perf_participant_count.non_rate_adapted.max) {
            perf_participant_count.non_rate_adapted.max = perf_combo.scale;
            perf_combo.params.populate_str();
            perf_participant_count.non_rate_adapted.max_params = perf_combo.params;
        }
    } else if (perf_participant_count.rate_adapted.initialized && (perf_combo.params.rep_mode == "RA-R" || perf_combo.params.rep_mode == "RA-SR")) {
        if (perf_combo.scale < perf_participant_count.rate_adapted.min) {
            perf_participant_count.rate_adapted.min = perf_combo.scale;
            perf_combo.params.populate_str();
            perf_participant_count.rate_adapted.min_params = perf_combo.params;
        }
        if (perf_combo.scale > perf_participant_count.rate_adapted.max) {
            perf_participant_count.rate_adapted.max = perf_combo.scale;
            perf_combo.params.populate_str();
            perf_participant_count.rate_adapted.max_params = perf_combo.params;
        }
    }
}

PerfCombo process_combination(PerfParticipantCount& perf_participant_count, PerfComboParams& params,
        const std::chrono::time_point<std::chrono::high_resolution_clock>& start_time, long long unsigned int& count) {
    // Perform checks to skip invalid combinations
    int num_receive_streams = params.num_senders * (params.num_participants - 1);
    assert(0 <= params.num_quality_high <= num_receive_streams);
    assert(0 <= params.num_quality_mid  <= num_receive_streams);
    assert(0 <= params.num_quality_low  <= num_receive_streams);
    assert(params.num_quality_high + params.num_quality_mid + params.num_quality_low == num_receive_streams);
    assert(0.0 <= params.loss_rate && params.loss_rate <= 20.0);
    assert(0.0 <= params.retx_overhead && params.retx_overhead <= params.loss_rate);

    if (params.rep_mode == "NRA") {
        assert(params.num_quality_high == 0 || params.num_quality_high == num_receive_streams);
        assert(params.num_quality_mid == 0 || params.num_quality_mid == num_receive_streams);
        assert(params.num_quality_low == 0 || params.num_quality_low == num_receive_streams);
        assert(params.num_quality_high == num_receive_streams || params.num_quality_mid == num_receive_streams
                || params.num_quality_low == num_receive_streams);
        if (params.sr_mode == "None") {
            assert(params.num_quality_high == num_receive_streams);
            assert(params.loss_rate <= 5.0);
        }
        
    } else if (params.rep_mode == "RA-R") {
        assert(params.sr_mode != "None" && params.loss_rate > 0.0);
        assert(params.num_quality_high <= num_receive_streams - params.num_senders);
        assert((params.num_quality_high > 0) + (params.num_quality_mid > 0) + (params.num_quality_low > 0) >= 2);
        assert((params.num_quality_high % params.num_senders == 0) + (params.num_quality_mid % params.num_senders == 0)
                + (params.num_quality_low % params.num_senders == 0) == 3);

    } else if (params.rep_mode == "RA-SR") {
        assert(params.sr_mode != "None" && params.loss_rate > 0.0);
        assert((params.num_quality_high > 0) + (params.num_quality_mid > 0) + (params.num_quality_low > 0) >= 2);
    }

    // Process a given combination of parameters
    PerfCombo perf_combo = compute_perf_for_combo(params);
    check_and_initialize(perf_participant_count, perf_combo);
    check_and_update(perf_participant_count, perf_combo);
    perf_participant_count.combo_count++;

    // Accounting and reporting
    count++;
    if (count % 100000000 == 0) {
        cout << "Elapsed: " << elapsed_time(start_time) << ", Tofino: " << params.tf_version << ", #Participants: " << params.num_participants
                << ", #Total combinations: " << count << ", #Processed combinations: " << perf_participant_count.combo_count
                << ", Current combo: " << params.params_str << endl;
    }

    return perf_combo;
}

PerfParticipantCount compute_perf_results_for_participant_count(int tf_version, int num_participants) {
    // Compute the performance of the Scallop data plane for a given number of participants
    PerfParticipantCount perf_participant_count;
    auto start_time = std::chrono::high_resolution_clock::now();
    long long unsigned int count = 0;

    // Combinations:
    // High-level:
    // * Tofino versions: 1, 2
    // * Participants (M): 2 to 100
    // Low-level:
    // * Senders per meeting (N): 1 to M
    // * Replication modes: NRA, RA-R, RA-SR
    // * Sequence number rewriting modes: S-LM, S-LR
    // * High-quality streams (h): 0 to N(M-1)
    // * Mid-quality streams (m): 0 to N(M-1) - h
    // * Low-quality streams (l): N(M-1) - h - m
    // * Loss rate (L): 0 to 20% (intervals of 1%)
    // * Retransmission overhead: 0 to L (intervals of 1%)

    // Generate the performance parameters for a given number of participants
    for (int num_senders = num_participants; num_senders >= 1; num_senders--) {
        int num_receive_streams = num_senders * (num_participants - 1);
        for (string rep_mode : {"NRA", "RA-R", "RA-SR"}) {
            for (string sr_mode : {"None", "S-LM", "S-LR"}) {
                for (float loss_rate = 20.0; loss_rate >= 0.0; loss_rate--) {

                    // Find maximum retransmission overhead
                    float max_retx_overhead = loss_rate;
                    if (sr_mode == "S-LR")
                        max_retx_overhead = round(0.5 * loss_rate);

                    for (float retx_overhead = max_retx_overhead; retx_overhead >= 0.0; retx_overhead--) {
                        // Quality distribution for NRA
                        if (rep_mode == "NRA") {
                            for (int num_high_quality = num_receive_streams; num_high_quality >= 0; num_high_quality -= num_receive_streams) {
                                for (int num_mid_quality =  num_receive_streams - num_high_quality; num_mid_quality >= 0; num_mid_quality -= num_receive_streams) {
                                    int num_low_quality = num_receive_streams - num_high_quality - num_mid_quality;
                                    // cout << "\t High: " << num_high_quality << ", Mid: " << num_mid_quality << ", Low: " << num_low_quality << endl;
                                    // Identify invalid conditions for NRA
                                    if (sr_mode == "None" && num_high_quality != num_receive_streams)
                                        continue;
                                    if (sr_mode == "None" && num_high_quality == num_receive_streams && loss_rate > 5.0)
                                        continue;
                                    // Create the parameter combination
                                    PerfComboParams params = {
                                        tf_version, num_participants, num_senders, rep_mode, sr_mode,
                                        num_high_quality, num_mid_quality, num_low_quality, loss_rate, retx_overhead
                                    };
                                    params.populate_str();
                                    // cout << "Params: " << params.params_str << endl;
                                    PerfCombo perf_combo = process_combination(perf_participant_count, params, start_time, count);
                                }
                            }
                            continue;
                        }

                        // Invalid conditions for RA-R and RA-SR
                        if (rep_mode == "RA-R" || rep_mode == "RA-SR") {
                            if (sr_mode == "None")
                                continue;
                            if (loss_rate == 0.0)
                                continue;
                        }

                        // Quality distribution for RA-R
                        if (rep_mode == "RA-R") {
                            for (int num_high_quality = num_receive_streams - num_senders; num_high_quality >= 0; num_high_quality = num_high_quality - num_senders) {
                                for (int num_mid_quality = num_receive_streams - num_high_quality; num_mid_quality >= 0; num_mid_quality = num_mid_quality - num_senders) {
                                    int num_low_quality = num_receive_streams - num_high_quality - num_mid_quality;
                                    // At least 2 qualities must be present
                                    if ((num_high_quality > 0) + (num_mid_quality > 0) + (num_low_quality > 0) < 2)
                                        continue;
                                    // All qualities must be a multiple of the number of senders
                                    if ((num_high_quality % num_senders == 0) + (num_mid_quality % num_senders == 0)
                                            + (num_low_quality % num_senders == 0) != 3)
                                        continue;
                                    // Create the parameter combination
                                    PerfComboParams params = {
                                        tf_version, num_participants, num_senders, rep_mode, sr_mode,
                                        num_high_quality, num_mid_quality, num_low_quality, loss_rate, retx_overhead
                                    };
                                    params.populate_str();
                                    PerfCombo perf_combo = process_combination(perf_participant_count, params, start_time, count);
                                }
                            }
                            continue;
                        }

                        // Quality distribution for RA-SR
                        if (rep_mode == "RA-SR") {
                            // Set step size such that not more than 100 combinations are generated
                            int step = 1;
                            if (num_receive_streams > 100)
                                step = num_receive_streams/100;
                            for (int num_high_quality = num_receive_streams; num_high_quality >= 0; num_high_quality = num_high_quality - step) {
                                for (int num_mid_quality = num_receive_streams - num_high_quality; num_mid_quality >= 0; num_mid_quality = num_mid_quality - step) {
                                    int num_low_quality = num_receive_streams - num_high_quality - num_mid_quality;
                                    // At least 2 qualities must be present
                                    if ((num_high_quality > 0) + (num_mid_quality > 0) + (num_low_quality > 0) < 2)
                                        continue;
                                    // Create the parameter combination
                                    PerfComboParams params = {
                                        tf_version, num_participants, num_senders, rep_mode, sr_mode,
                                        num_high_quality, num_mid_quality, num_low_quality, loss_rate, retx_overhead
                                    };
                                    params.populate_str();
                                    PerfCombo perf_combo = process_combination(perf_participant_count, params, start_time, count);
                                }
                            }
                            continue;
                        }
                    }
                }
            }
        }
    }
    return perf_participant_count;
}

void write_results_to_file_for_tofino_version(map<int, PerfParticipantCount>& results, int tofino_version) {
    // Write the performance results to a CSV file
    ofstream file("data/tofino" + to_string(tofino_version) + ".csv");
    if (!file.is_open()) {
        cout << "Error opening file" << endl;
        exit(1);
    }
    // Write the header
    file << "num_participants,count_sw_min,count_sw_max,count_sw_min_params,count_sw_max_params,";
    file << "count_hw_bw_min,count_hw_bw_max,count_hw_bw_min_params,count_hw_bw_max_params,";
    file << "count_hw_rep_min,count_hw_rep_max,count_hw_rep_min_params,count_hw_rep_max_params,";
    file << "count_hw_nra_min,count_hw_nra_max,count_hw_nra_min_params,count_hw_nra_max_params,";
    file << "count_hw_rar_min,count_hw_rar_max,count_hw_rar_min_params,count_hw_rar_max_params,";
    file << "count_hw_rasr_min,count_hw_rasr_max,count_hw_rasr_min_params,count_hw_rasr_max_params,";
    file << "count_hw_mem_min,count_hw_mem_max,count_hw_mem_min_params,count_hw_mem_max_params,";
    file << "count_hw_slm_min,count_hw_slm_max,count_hw_slm_min_params,count_hw_slm_max_params,";
    file << "count_hw_slr_min,count_hw_slr_max,count_hw_slr_min_params,count_hw_slr_max_params,";
    file << "count_hw_min,count_hw_max,count_hw_min_params,count_hw_max_params,";
    file << "scale_nra_min,scale_nra_max,scale_nra_min_params,scale_nra_max_params,";
    file << "scale_ra_min,scale_ra_max,scale_ra_min_params,scale_ra_max_params,";
    file << "scale_all_min,scale_all_max,scale_all_min_params,scale_all_max_params" << endl;
    // Write the results
    for (auto& [num_participants, perf_participant_count] : results) {
        file << num_participants << ","
            << perf_participant_count.software.min << ","
            << perf_participant_count.software.max << ","
            << perf_participant_count.software.min_params.params_str << ","
            << perf_participant_count.software.max_params.params_str << ","
            << perf_participant_count.hardware_bw.min << ","
            << perf_participant_count.hardware_bw.max << ","
            << perf_participant_count.hardware_bw.min_params.params_str << ","
            << perf_participant_count.hardware_bw.max_params.params_str << ","
            << perf_participant_count.hardware_rep.min << ","
            << perf_participant_count.hardware_rep.max << ","
            << perf_participant_count.hardware_rep.min_params.params_str << ","
            << perf_participant_count.hardware_rep.max_params.params_str << ","
            << perf_participant_count.hardware_nra.min << ","
            << perf_participant_count.hardware_nra.max << ","
            << perf_participant_count.hardware_nra.min_params.params_str << ","
            << perf_participant_count.hardware_nra.max_params.params_str << ","
            << perf_participant_count.hardware_rar.min << ","
            << perf_participant_count.hardware_rar.max << ","
            << perf_participant_count.hardware_rar.min_params.params_str << ","
            << perf_participant_count.hardware_rar.max_params.params_str << ","
            << perf_participant_count.hardware_rasr.min << ","
            << perf_participant_count.hardware_rasr.max << ","
            << perf_participant_count.hardware_rasr.min_params.params_str << ","
            << perf_participant_count.hardware_rasr.max_params.params_str << ","
            << perf_participant_count.hardware_mem.min << ","
            << perf_participant_count.hardware_mem.max << ","
            << perf_participant_count.hardware_mem.min_params.params_str << ","
            << perf_participant_count.hardware_mem.max_params.params_str << ","
            << perf_participant_count.hardware_slm.min << ","
            << perf_participant_count.hardware_slm.max << ","
            << perf_participant_count.hardware_slm.min_params.params_str << ","
            << perf_participant_count.hardware_slm.max_params.params_str << ","
            << perf_participant_count.hardware_slr.min << ","
            << perf_participant_count.hardware_slr.max << ","
            << perf_participant_count.hardware_slr.min_params.params_str << ","
            << perf_participant_count.hardware_slr.max_params.params_str << ","
            << perf_participant_count.hardware.min << ","
            << perf_participant_count.hardware.max << ","
            << perf_participant_count.hardware.min_params.params_str << ","
            << perf_participant_count.hardware.max_params.params_str << ","
            << perf_participant_count.non_rate_adapted.min << ","
            << perf_participant_count.non_rate_adapted.max << ","
            << perf_participant_count.non_rate_adapted.min_params.params_str << ","
            << perf_participant_count.non_rate_adapted.max_params.params_str << ","
            << perf_participant_count.rate_adapted.min << ","
            << perf_participant_count.rate_adapted.max << ","
            << perf_participant_count.rate_adapted.min_params.params_str << ","
            << perf_participant_count.rate_adapted.max_params.params_str << ","
            << perf_participant_count.all.min << ","
            << perf_participant_count.all.max << ","
            << perf_participant_count.all.min_params.params_str << ","
            << perf_participant_count.all.max_params.params_str << endl;
    }
    file.close();
    cout << "Results for Tofino " << tofino_version << " written to file" << endl;
    return;
}

void write_results_to_files(PerfResults& results) {
    // Write the performance results to separate CSV files for Tofino1 and Tofino2
    write_results_to_file_for_tofino_version(results.tofino1, 1);
    write_results_to_file_for_tofino_version(results.tofino2, 2);
    return;
}

void compute_scallop_perf(int min_participants=2, int max_participants=100) {
    // Compute the performance of the Scallop data plane    
    long long unsigned int count = 0;
    auto start_time = std::chrono::high_resolution_clock::now();
    PerfResults results;

    for (int tf_v : {2, 1}) {
        for (int num_participants = max_participants; num_participants >= min_participants; num_participants--) {
            PerfParticipantCount perf_participant_count = compute_perf_results_for_participant_count(tf_v, num_participants);
            if (tf_v == 1) {
                results.tofino1[num_participants] = perf_participant_count;
            } else if (tf_v == 2) {
                results.tofino2[num_participants] = perf_participant_count;
            }
            count = count + perf_participant_count.combo_count;
            cout << "[Total] Elapsed: " << elapsed_time(start_time) << ", Tofino version: " << tf_v << ", No. of participants: " << num_participants
                 << ", Local #combinations: " << perf_participant_count.combo_count << ", Global #combinations: " << count << endl;
        }
    }
    cout << "Total combinations: " << count << endl;

    write_results_to_files(results);
}

/******************** Main function ********************/

int main() {
    // Calculate the performance of the Scallop data plane
    compute_scallop_perf(2, 4);
    return 0;
}