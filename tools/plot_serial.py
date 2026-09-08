import argparse
import re
from collections import deque

import matplotlib.pyplot as plt
import serial


def parse_line(line):
    values = {}

    for name, value in re.findall(r"([A-Za-z_]+):([-+]?\d+(?:\.\d+)?)", line):
        values[name] = float(value)

    return values


def main():
    parser = argparse.ArgumentParser(description="Grafica datos serie del hotend.")
    parser.add_argument("port", help="Puerto serial, por ejemplo COM3")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--samples", type=int, default=300)
    args = parser.parse_args()

    ser = serial.Serial(args.port, args.baud, timeout=1)

    xs = deque(maxlen=args.samples)
    temperaturas = deque(maxlen=args.samples)
    pwms = deque(maxlen=args.samples)

    plt.ion()
    fig, ax = plt.subplots()
    temp_line, = ax.plot([], [], label="Temperatura C")
    pwm_line, = ax.plot([], [], label="PWM")

    ax.set_ylim(0, 300)
    ax.set_xlabel("Muestras")
    ax.set_ylabel("Valor")
    ax.grid(True)
    ax.legend()

    sample = 0

    while plt.fignum_exists(fig.number):
        raw = ser.readline().decode(errors="ignore").strip()

        if not raw:
            continue

        data = parse_line(raw)

        if "temperatura" not in data:
            continue

        sample += 1
        xs.append(sample)
        temperaturas.append(data["temperatura"])
        pwms.append(data.get("pwm", 0.0))

        temp_line.set_data(xs, temperaturas)
        pwm_line.set_data(xs, pwms)

        ax.set_xlim(max(0, sample - args.samples), max(args.samples, sample))
        fig.canvas.draw()
        fig.canvas.flush_events()

    ser.close()


if __name__ == "__main__":
    main()
