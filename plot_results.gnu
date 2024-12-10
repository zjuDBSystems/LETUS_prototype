set terminal pngcairo enhanced font "arial,12" size 1200,800
set style data linespoints
set grid

# Latency plots
set output 'results/latency_vs_batch_size.png'
set title 'Average Latency vs Batch Size'
set xlabel 'Batch Size'
set ylabel 'Latency (s)'
set key outside right
plot for [vs in "256 512 1024 2048"] \
    'results/results.csv' using 1:3 every ::1 \
    title sprintf('Put (Value Size=%s)', vs), \
    '' using 1:4 title sprintf('Get (Value Size=%s)', vs)

set output 'results/latency_vs_value_size.png'
set title 'Average Latency vs Value Size'
set xlabel 'Value Size (bytes)'
set ylabel 'Latency (s)'
set key outside right
plot for [bs in "500 1000 2000 4000"] \
    'results/results.csv' using 2:3 every ::1 \
    title sprintf('Put (Batch Size=%s)', bs), \
    '' using 2:4 title sprintf('Get (Batch Size=%s)', bs)

# Throughput plots
set output 'results/throughput_vs_batch_size.png'
set title 'Throughput vs Batch Size'
set xlabel 'Batch Size'
set ylabel 'Throughput (ops/s)'
set key outside right
plot for [vs in "256 512 1024 2048"] \
    'results/results.csv' using 1:5 every ::1 \
    title sprintf('Put (Value Size=%s)', vs), \
    '' using 1:6 title sprintf('Get (Value Size=%s)', vs)

set output 'results/throughput_vs_value_size.png'
set title 'Throughput vs Value Size'
set xlabel 'Value Size (bytes)'
set ylabel 'Throughput (ops/s)'
set key outside right
plot for [bs in "500 1000 2000 4000"] \
    'results/results.csv' using 2:5 every ::1 \
    title sprintf('Put (Batch Size=%s)', bs), \
    '' using 2:6 title sprintf('Get (Batch Size=%s)', bs)