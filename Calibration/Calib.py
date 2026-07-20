"""
Load Cell Easy Calibration Tool - Guided Workflow
====================================================
Talks to the Teensy over serial using the command protocol built into
loadcell_firmware.ino (CH:, TARE:, CAL:, STREAM:RAW/WEIGHT/OFF, STATUS).
No firmware changes needed - this script drives the whole sequence:

    1) Select COM port
    2) Select load cell (physical unit under test)
    3) Select ADC channel (CH12 / CH34 / CH56 / CH78 / CH9)
    4) Tare that channel
    5) Run calibration (collect known weights, fit slope/intercept)
    6) Save EVERYTHING into one Excel file:
         Sheet1 "Raw_Data"    -> raw ADC samples per weight
         Sheet2 "Calibration" -> weight/ADC table, M, C, R^2, embedded slope graph
    7) Push the fitted M/C back to the board (CAL:) and stream live
       weight (STREAM:WEIGHT) so you can immediately check it against
       a known/unknown weight.

Run it top to bottom for one load cell/channel, or loop back for the
next one when prompted at the end.
"""

import os
import sys
import time
from datetime import datetime

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import serial
import serial.tools.list_ports
from openpyxl.drawing.image import Image as XLImage

# ==========================================
# CONFIG
# ==========================================

SAVE_FOLDER = r"D:\DBB\LOAD_cell\Calibration\values"
BAUDRATE = 115200
SAMPLES_PER_WEIGHT = 2400
TARE_SAMPLES = 2400
SERIAL_TIMEOUT = 2

CHANNEL_LABELS = {
    "1": ("CH12", 0),
    "2": ("CH34", 1),
    "3": ("CH56", 2),
    "4": ("CH78", 3),
    "5": ("CH9", 4),
}

os.makedirs(SAVE_FOLDER, exist_ok=True)


# ==========================================
# SERIAL HELPERS
# ==========================================

def list_available_ports():
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        print("No serial ports detected.")
        return
    print("\nDetected ports:")
    for p in ports:
        print(f"  {p.device}  -  {p.description}")


def open_port():
    list_available_ports()
    port = input("\nEnter COM Port (e.g. COM4 or 4): ").strip()
    if not port.upper().startswith("COM"):
        port = f"COM{port}"
    try:
        ser = serial.Serial(port, BAUDRATE, timeout=SERIAL_TIMEOUT)
        time.sleep(2)  # allow Teensy to reset / settle after opening port
        ser.reset_input_buffer()
        print(f"Connected to {port}")
        return ser
    except Exception as e:
        print("\nUnable to open serial port")
        print(e)
        raise SystemExit(1)


def send_command(ser, cmd, wait_s=3):
    """Send a command, wait for a response line starting with OK or ERR."""
    ser.reset_input_buffer()
    ser.write((cmd + "\n").encode())
    deadline = time.time() + wait_s
    while time.time() < deadline:
        line = ser.readline().decode(errors="ignore").strip()
        if not line:
            continue
        if line.startswith("OK:") or line.startswith("ERR:"):
            return line
    return None


def stop_stream(ser):
    send_command(ser, "STREAM:OFF")


# ==========================================
# GUIDED SELECTION STEPS
# ==========================================

def select_loadcell():
    print("\n--- Step: Select Load Cell ---")

    while True:
        try:
            number = int(input("Enter Load Cell Number (e.g. 1, 2, 5, 10, ...): ").strip())

            if number > 0:
                return f"LC{number}"
            else:
                print("Please enter a positive number.")

        except ValueError:
            print("Invalid input. Please enter a valid integer.")


def select_channel(ser):
    print("\n--- Step: Select ADC Channel ---")
    for key, (label, _idx) in CHANNEL_LABELS.items():
        print(f"  {key}) {label}")
    choice = input("Select channel: ").strip()
    while choice not in CHANNEL_LABELS:
        choice = input("Invalid choice, try again: ").strip()
    label, idx = CHANNEL_LABELS[choice]
    resp = send_command(ser, f"CH:{idx}")
    print(f"-> {resp}")
    return label


def do_tare(ser):
    print(f"\n--- Step: Tare ({TARE_SAMPLES} samples) ---")
    input("Remove all weight from the load cell, then press ENTER...")
    resp = send_command(ser, f"TARE:{TARE_SAMPLES}", wait_s=10)
    print(f"-> {resp}")
    if not resp or not resp.startswith("OK:"):
        print("WARNING: tare did not confirm OK - check wiring/board before continuing.")
    return resp


# ==========================================
# CALIBRATION COLLECTION
# ==========================================

def collect_raw_stream(ser, num_samples, label=""):
    """Switch board to RAW streaming and capture num_samples ADC counts."""
    send_command(ser, "STREAM:RAW")
    samples = []
    ser.reset_input_buffer()
    while len(samples) < num_samples:
        line = ser.readline().decode(errors="ignore").strip()
        if not line:
            continue
        try:
            samples.append(float(line))
            if len(samples) % 200 == 0:
                print(f"{label} {len(samples)}/{num_samples}", end="\r")
        except ValueError:
            continue  # skip status lines like OK:/ERR:
    stop_stream(ser)
    print()
    return samples


def collect_calibration_points(ser):
    adc_data = {}
    weights = []

    print("\n--- Step: Calibration ---")
    print("Enter known weights one at a time (kg). Type q when finished.\n")

    while True:
        w = input("Weight (kg): ").strip().lower()
        if w in ["q", "quit", "done", "exit"]:
            break
        try:
            weight = float(w)
        except ValueError:
            print("Invalid weight.")
            continue

        input(f"\nPlace {weight} kg on the load cell and press ENTER...")
        print(f"Collecting {SAMPLES_PER_WEIGHT} samples...")

        samples = collect_raw_stream(ser, SAMPLES_PER_WEIGHT, label=f"{weight}kg")

        adc_data[weight] = samples
        weights.append(weight)

        avg = np.mean(samples)
        print(f"Completed {weight} kg | Average ADC = {avg:.2f}\n")

    return weights, adc_data


def fit_calibration(weights, adc_data):
    avg_adc = [np.mean(adc_data[w]) for w in weights]
    M, C = np.polyfit(weights, avg_adc, 1)

    y_pred = M * np.array(weights) + C
    ss_res = np.sum((np.array(avg_adc) - y_pred) ** 2)
    ss_tot = np.sum((np.array(avg_adc) - np.mean(avg_adc)) ** 2)
    R2 = 1 - (ss_res / ss_tot) if ss_tot != 0 else float("nan")

    return M, C, R2, avg_adc


def push_calibration(ser, M, C):
    print("\n--- Step: Applying calibration to firmware ---")
    resp = send_command(ser, f"CAL:{M:.6f},{C:.6f}")
    print(f"-> {resp}")
    return resp


# ==========================================
# VERIFICATION (live weight readout)
# ==========================================

def verify_live(ser):
    print("\n--- Step: Verify Live Weight ---")
    print("Calibration has been applied successfully.")
    print("Place a known or unknown weight on the load cell.")
    print("Live weight will continue until you press Ctrl+C.\n")

    send_command(ser, "STREAM:WEIGHT")
    ser.reset_input_buffer()

    try:
        while True:
            line = ser.readline().decode(errors="ignore").strip()
            if not line:
                continue

            try:
                weight = float(line)
                print(f"Weight: {weight:8.3f} kg", end="\r")
            except ValueError:
                pass

    except KeyboardInterrupt:
        print("\nVerification stopped by user.")

    finally:
        stop_stream(ser)
        print()


# ==========================================
# SAVE RESULTS - ONE EXCEL FILE
# Sheet1 "Raw_Data"    : raw samples per weight
# Sheet2 "Calibration" : weight/ADC table + M/C/R2 + embedded slope graph
# ==========================================

def save_results(loadcell_id, channel_label, weights, adc_data, M, C, R2, avg_adc):
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")

    raw_df = pd.DataFrame()
    for w in weights:
        raw_df[f"{w}_kg"] = pd.Series(adc_data[w])

    cal_df = pd.DataFrame({"Weight_kg": weights, "Average_ADC": avg_adc})
    cal_df["Slope_M"] = M
    cal_df["Intercept_C"] = C
    cal_df["R2"] = R2
    cal_df["Channel"] = channel_label
    cal_df["LoadCell"] = loadcell_id

    excel_file = os.path.join(
        SAVE_FOLDER, f"{loadcell_id}_{channel_label}_Calibration_{timestamp}.xlsx"
    )

    # Build the graph first so it can be embedded into Sheet2
    fig, ax = plt.subplots(figsize=(7, 4.5))
    ax.scatter(weights, avg_adc, s=70, label="Measured", zorder=3)
    x_fit = np.linspace(min(weights), max(weights), 100)
    y_fit = M * x_fit + C
    ax.plot(x_fit, y_fit, "--", linewidth=2, label="Linear Fit", zorder=2)
    ax.set_xlabel("Weight (kg)")
    ax.set_ylabel("Average ADC")
    ax.set_title(f"Load Cell {loadcell_id} ({channel_label}) Calibration")
    ax.grid(True, alpha=0.4)
    ax.legend()
    ax.text(
        0.05, 0.95,
        f"M = {M:.4f}\nC = {C:.4f}\nR\u00b2 = {R2:.5f}",
        transform=ax.transAxes,
        verticalalignment="top",
        bbox=dict(facecolor="white", alpha=0.85),
    )
    graph_png = os.path.join(
        SAVE_FOLDER, f"_tmp_{loadcell_id}_{channel_label}_{timestamp}.png"
    )
    fig.savefig(graph_png, dpi=200, bbox_inches="tight")
    plt.close(fig)

    with pd.ExcelWriter(excel_file, engine="openpyxl") as writer:
        raw_df.to_excel(writer, sheet_name="Raw_Data", index=False)
        cal_df.to_excel(writer, sheet_name="Calibration", index=False)

        # Embed the slope graph into Sheet2, to the right of the values table
        ws = writer.sheets["Calibration"]
        img = XLImage(graph_png)
        img.width = 560
        img.height = 360
        anchor_col = len(cal_df.columns) + 2  # leave a gap column after the table
        anchor_cell = f"{chr(ord('A') + anchor_col)}2"
        ws.add_image(img, anchor_cell)

    os.remove(graph_png)  # temp file no longer needed, it's embedded now

    return excel_file


# ==========================================
# MAIN - GUIDED SEQUENTIAL FLOW
# ==========================================

def run_one_cycle(ser):
    loadcell_id = select_loadcell()
    channel_label = select_channel(ser)
    do_tare(ser)

    weights, adc_data = collect_calibration_points(ser)
    if len(weights) < 2:
        print("\nNeed at least two calibration weights - aborting this cycle.")
        return

    M, C, R2, avg_adc = fit_calibration(weights, adc_data)
    print("\n--- Calibration Result ---")
    print(f"M = {M:.6f}")
    print(f"C = {C:.6f}")
    print(f"R2 = {R2:.6f}")

    excel_file = save_results(loadcell_id, channel_label, weights, adc_data, M, C, R2, avg_adc)
    print(f"\nSaved: {excel_file}")

    push_calibration(ser, M, C)
    verify_live(ser)


def main():
    ser = open_port()

    resp = send_command(ser, "STATUS", wait_s=5)
    print(f"Board status -> {resp}")

    while True:
        run_one_cycle(ser)
        again = input("\nCalibrate another load cell / channel? (y/n): ").strip().lower()
        if again != "y":
            break

    stop_stream(ser)
    ser.close()
    print("Done.")


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\nInterrupted by user.")
        sys.exit(0)