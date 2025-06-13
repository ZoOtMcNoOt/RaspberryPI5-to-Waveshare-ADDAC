"""Plots ADC scan data from a CSV file, converting raw values to voltage."""

import os
import pandas as pd
import matplotlib.pyplot as plt

# Define the CSV filename
CSV_FILENAME = "adc_scan_data.csv"

# ADC Configuration Constants (adjust if your C code uses different values)
VREF = 5.0  # Reference voltage (e.g., 5.0V)
MAX_ADC_VALUE_POSITIVE = 8388607.0  # For a 24-bit ADC, this is 2^23 - 1 (signed)
# If PGA gain is not 1, adjust here or ensure raw values account for it.
PGA_GAIN = 1.0

def plot_adc_data(csv_filepath):
    """
    Reads ADC scan data from a CSV file, converts raw values to voltage,
    and plots each channel.

    Args:
        csv_filepath (str): The path to the CSV file.
    """
    try:
        # Read the CSV file into a pandas DataFrame
        data_frame = pd.read_csv(csv_filepath)
    except FileNotFoundError:
        print(f"Error: The file {csv_filepath} was not found.")
        return
    except pd.errors.EmptyDataError:
        print(f"Error: The file {csv_filepath} is empty.")
        return
    except pd.errors.ParserError as parse_ex:
        print(f"Error parsing CSV file {csv_filepath}: {parse_ex}")
        return
    except IOError as io_ex:  # More specific exception for IO issues
        print(f"An IO error occurred while reading the CSV file {csv_filepath}: {io_ex}")
        return
    except Exception as ex:  # Catching other potential unexpected errors
        print(f"An unexpected error occurred: {ex}")
        return

    if data_frame.empty:
        print(f"The DataFrame loaded from {csv_filepath} is empty. Nothing to plot.")
        return

    # Set the 'SampleSet' column as the index (x-axis)
    if "SampleSet" not in data_frame.columns:
        print("Error: 'SampleSet' column not found in the CSV file.")
        return
    
    data_frame = data_frame.set_index("SampleSet")

    # Get the list of AIN channel columns to plot
    ain_columns = [col for col in data_frame.columns if col.startswith("AIN")]

    if not ain_columns:
        print("No AIN columns found to plot (e.g., AIN0, AIN1, ...).")
        return

    # Create the plot
    plt.figure(figsize=(12, 6))

    for column in ain_columns:
        # Convert raw ADC values to voltage
        # Formula: Voltage = (RawValue / (2^23 - 1)) * Vref / PGA_GAIN
        voltage_values = (data_frame[column] / MAX_ADC_VALUE_POSITIVE) * VREF / PGA_GAIN
        plot_label = f"{column} (Voltage)"
        plt.plot(data_frame.index, voltage_values, label=plot_label)

    plt.title("ADC Scan Data (Voltage)")
    plt.xlabel("Sample Set")
    plt.ylabel("Voltage (V)")
    plt.legend()
    plt.grid(True)
    plt.tight_layout() # Adjust layout to prevent labels from overlapping
    plt.show()

if __name__ == "__main__":
    # Construct the full path to the CSV file assuming the script
    # is in the same directory as the CSV file.
    script_dir = os.path.dirname(os.path.abspath(__file__))
    csv_file_path = os.path.join(script_dir, CSV_FILENAME)
    plot_adc_data(csv_file_path)
