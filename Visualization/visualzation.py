import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

def analyze_latency(csv_filenames):
    cutoff = 5000
    bin_width = 50
    bins = list(range(0, cutoff, bin_width))

    plt.figure(figsize=(12, 6))

    for csv_filename in csv_filenames:
        df = pd.read_csv(csv_filename, header=None, names=['OrderType', 'Latency'])
        df_filtered = df[df['Latency'] <= cutoff]

        median_latency = df_filtered['Latency'].median()
        variance_latency = df_filtered['Latency'].var(ddof=1)  # sample variance
        excluded_count = len(df) - len(df_filtered)

        # Histogram counts
        counts, bin_edges = np.histogram(df_filtered['Latency'], bins=bins)

        # Draw filled histogram area and capture the PolyCollection object
        poly = plt.fill_between(
            bin_edges[:-1], counts, step='pre', alpha=0.5,
            label=(f'{csv_filename}\n'
                   f'Median={median_latency:.1f} ns, Var={variance_latency:.1f}, '
                   f'Excluded={excluded_count}')
        )

        # Use the same facecolor for the median line
        color = poly.get_facecolor()[0]   # RGBA tuple
        plt.axvline(median_latency, linestyle='dotted', linewidth=2, color=color)

    plt.title('Latency Distribution – Filled Histogram Comparison')
    plt.xlabel(f'Latency (ns) (>{cutoff} excluded)')
    plt.ylabel('Frequency')
    plt.legend(loc='upper right', fontsize=9)
    plt.tight_layout()
    plt.savefig('latency_filled_histogram_comparison.png')
    plt.show()

if __name__ == "__main__":
    files = ["benchmark_time_model1.csv", "benchmark_time_model2.csv", "benchmark_time_model3.csv"]
    analyze_latency(files)

