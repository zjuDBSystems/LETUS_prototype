import pandas as pd
import matplotlib.pyplot as plt
import numpy as np


df = pd.read_csv('results/results.csv')

plt.figure(figsize=(15, 10))


plt.subplot(2, 2, 1)
for value_size in sorted(df['value_size'].unique()):
    data = df[df['value_size'] == value_size]
    plt.plot(data['batch_size'].to_numpy(), data['put_latency'].to_numpy(), 
             marker='o', label=f'Value Size={value_size}B')

plt.title('Put Latency vs Batch Size')
plt.xlabel('Batch Size')
plt.ylabel('Latency (s)')
plt.legend()
plt.grid(True)

plt.subplot(2, 2, 2)
for value_size in sorted(df['value_size'].unique()):
    data = df[df['value_size'] == value_size]
    plt.plot(data['batch_size'].to_numpy(), data['get_latency'].to_numpy(), 
             marker='o', label=f'Value Size={value_size}B')

plt.title('Get Latency vs Batch Size')
plt.xlabel('Batch Size')
plt.ylabel('Latency (s)')
plt.legend()
plt.grid(True)

plt.subplot(2, 2, 3)
for value_size in sorted(df['value_size'].unique()):
    data = df[df['value_size'] == value_size]
    plt.plot(data['batch_size'].to_numpy(), data['put_throughput'].to_numpy(), 
             marker='o', label=f'Value Size={value_size}B')

plt.title('Put Throughput vs Batch Size')
plt.xlabel('Batch Size')
plt.ylabel('Throughput (ops/s)')
plt.legend()
plt.grid(True)

plt.subplot(2, 2, 4)
for value_size in sorted(df['value_size'].unique()):
    data = df[df['value_size'] == value_size]
    plt.plot(data['batch_size'].to_numpy(), data['get_throughput'].to_numpy(), 
             marker='o', label=f'Value Size={value_size}B')

plt.title('Get Throughput vs Batch Size')
plt.xlabel('Batch Size')
plt.ylabel('Throughput (ops/s)')
plt.legend()
plt.grid(True)

plt.tight_layout()

plt.savefig('results/performance_analysis.png', dpi=300, bbox_inches='tight')
plt.close()

print("图表已保存为 results/performance_analysis.png")