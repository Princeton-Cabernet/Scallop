#!/bin/bash

# List of useful log files:
# mau.resources.log
# parser.characterize.log
# phv_allocation_summary_0.log
# table_summary.log

# Clear previous results
> tofino_resources.txt

# Build logs directory path
logs_dir="build/p4-build/tofino/sfu/sfu/tofino/pipe/logs"
code_dir="p4-sfu/tofino/data_plane"
prog_name="sfu.p4"

# Function to count the total number of lines of code in files in a directory
sum_lines() {
    local directory="$1"
    local sum=0
    # Iterate over files and directories in the given directory
    for file in "$directory"/*; do
        if [ -d "$file" ]; then
            # If it's a directory, recursively call the function
            sum=$((sum + $(sum_lines "$file")))
        elif [ -f "$file" ] && [[ "$file" == *.p4 ]]; then
            # If it's a file with the ".p4" extension, add the number of lines to the sum
            sum=$((sum + $(wc -l < "$file")))
        fi
    done
    echo "$sum"
}

# Function to trim spaces in a string
trim() {
    local input_string="$1"
    local trimmed_string=$(echo "$input_string" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
    echo "$trimmed_string"
}

# Compute the number of lines of P4 code
echo "Lines of code: $(sum_lines $code_dir)" >> tofino_resources.txt

# Compile the latest source code
echo "Compiling the P4 program: $prog_name"
{ time ./p4_build.sh $code_dir/$prog_name; } 2>sfu_compile_time.txt

# Extract the real time from the time command output
compilation_real_time=$(grep "real" sfu_compile_time.txt | awk '{print $2}')
echo >> tofino_resources.txt
echo "Compilation runtime: $compilation_real_time" >> tofino_resources.txt

# Extract the parsing depth from the parser characterization log
ingress_parser_depth=$(grep -oE "Longest path \([0-9]+ states\) on ingress:" "$logs_dir/parser.characterize.log" | sed 's/Longest path (//;s/ states) on ingress://')
egress_parser_depth=$(grep -oE "Longest path \([0-9]+ states\) on egress:" "$logs_dir/parser.characterize.log" | sed 's/Longest path (//;s/ states) on egress://')
echo >> tofino_resources.txt
echo "Number of parser states in the longest path on ingress: $ingress_parser_depth" >> tofino_resources.txt
echo "Number of parser states in the longest path on egress: $egress_parser_depth" >> tofino_resources.txt

# Define Tofino1 resource limits
total_stages=12
exact_xbar_per_stage=128
ternary_xbar_per_stage=66
hash_bits_per_stage=416
hash_dist_per_stage=6
SRAM_per_stage=80
TCAM_per_stage=24
VLIW_per_stage=32
logical_table_per_stage=16

# Extract the number of stages from the table summary log
ingress_stages_count=$(grep -oE "Number of stages for ingress table allocation: ([0-9]+)" "$logs_dir/table_summary.log" | sed 's/Number of stages for ingress table allocation: //')
egress_stages_count=$(grep -oE "Number of stages for egress table allocation: ([0-9]+)" "$logs_dir/table_summary.log" | sed 's/Number of stages for egress table allocation: //')
echo >> tofino_resources.txt
echo "Number of ingress stages: $ingress_stages_count" >> tofino_resources.txt
echo "Number of egress stages: $egress_stages_count" >> tofino_resources.txt

# Extract the PHV usage from the PHV allocation summary log
PHV_usage=$(grep "Overall PHV Usage" "$logs_dir/phv_allocation_summary_0.log" | cut -d "|" -f3 | cut -d "(" -f2 | cut -d ")" -f1)
echo >> tofino_resources.txt
echo "PHV containers usage: $(trim $PHV_usage)%" >> tofino_resources.txt

# Extract match input crossbar usages from the MAU resources log
exact_xbar_usage=$(grep "Totals" "$logs_dir/mau.resources.log" | cut -d "|" -f3 | tr -d -c 0-9)
ternary_xbar_usage=$(grep "Totals" "$logs_dir/mau.resources.log" | cut -d "|" -f4 | tr -d -c 0-9)
exact_xbar_pct=$(echo "scale=2 ; $exact_xbar_usage * 100 / ($exact_xbar_per_stage * $total_stages)" | bc)%
ternary_xbar_pct=$(echo "scale=2 ; $ternary_xbar_usage * 100 / ($ternary_xbar_per_stage * $total_stages)" | bc)%
echo >> tofino_resources.txt
echo "Exact match crossbar usage: $(trim $exact_xbar_pct)" >> tofino_resources.txt
echo "Ternary match crossbar usage: $(trim $ternary_xbar_pct)" >> tofino_resources.txt

# Extract hash resource usages from the MAU resources log
hash_bit_usage=$(grep "Totals" "$logs_dir/mau.resources.log" | cut -d "|" -f5 | tr -d -c 0-9)
hash_dist_usage=$(grep "Totals" "$logs_dir/mau.resources.log" | cut -d "|" -f6 | tr -d -c 0-9)
hash_bit_pct=$(echo "scale=2 ; $hash_bit_usage * 100 / ($hash_bits_per_stage * $total_stages)" | bc)%
hash_dist_pct=$(echo "scale=2 ; $hash_dist_usage * 100 / ($hash_dist_per_stage * $total_stages)" | bc)%
echo >> tofino_resources.txt
echo "Hash bits usage: $(trim $hash_bit_pct)" >> tofino_resources.txt
echo "Hash distribution units usage: $(trim $hash_dist_pct)" >> tofino_resources.txt

# Extract the SRAM usage from the MAU resources log
SRAM_usage=$(grep "Totals" "$logs_dir/mau.resources.log" | cut -d "|" -f8 | tr -d -c 0-9)
SRAM_usage_pct=$(echo "scale=2 ; $SRAM_usage * 100 / ($SRAM_per_stage * $total_stages)" | bc)%
echo >> tofino_resources.txt
echo "SRAM usage: $(trim $SRAM_usage_pct)" >> tofino_resources.txt

# Extract the TCAM usage from the MAU resources log
TCAM_usage=$(grep "Totals" "$logs_dir/mau.resources.log" | cut -d "|" -f10 | tr -d -c 0-9)
TCAM_usage_pct=$(echo "scale=2 ; $TCAM_usage * 100 / ($TCAM_per_stage * $total_stages)" | bc)%
echo >> tofino_resources.txt
echo "TCAM usage: $(trim $TCAM_usage_pct)" >> tofino_resources.txt

# Extract the VLIW usage from the MAU resources log
VLIW_usage=$(grep "Totals" "$logs_dir/mau.resources.log" | cut -d "|" -f11 | tr -d -c 0-9)
VLIW_usage_pct=$(echo "scale=2 ; $VLIW_usage * 100 / ($VLIW_per_stage * $total_stages)" | bc)%
echo >> tofino_resources.txt
echo "VLIW instructions usage: $(trim $VLIW_usage_pct)" >> tofino_resources.txt

# Extract the logical table ID usage from the MAU resources log
logical_table_usage=$(grep "Totals" "$logs_dir/mau.resources.log" | cut -d "|" -f22 | tr -d -c 0-9)
logical_table_pct=$(echo "scale=2 ; $logical_table_usage * 100 / ($logical_table_per_stage * $total_stages)" | bc)%
echo >> tofino_resources.txt
echo "Logical table ID usage: $(trim $logical_table_pct)" >> tofino_resources.txt

# Print a summary
cat tofino_resources.txt