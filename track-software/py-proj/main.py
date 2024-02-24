import serial
from serial.tools.list_ports import comports 
from readchar import readkey
from rich.console import Console
from rich.prompt import Confirm
import sys
import os
import csv


WIDTH = 100
DEBUG = True

OUTPUT_FILENAME = "log.csv"

console = Console()

def main():
    port = get_port()
    point_list = []

    # Check if output file exists, warn user if so
    if os.path.isfile(f"./{OUTPUT_FILENAME}"):
        clear()
        console.print(f"[bold red]{'OUTPUT FILE (LOG.CSV) ALREADY EXISTS, ENSURE ANY DATA FROM PREVIOUS RECORDINGS IS SAVED ELSEWHERE AS IT WILL BE OVERWRITTEN (If you do not care about the data currently in the file, then ignore this warning and continue)'}")
        response = Confirm.ask("Do you want to overwrite the data?")
        if not response:
            quit()

    while True:
        
        clear() # Clear screen for next render
        buffer = render(point_list) # Render buffer
        console.print(buffer)     # Print buffer to screen
        if not handle_input(point_list, port):  # Get user input for next frame
            break

    if port:
        port.close()


def handle_input(point_list, port) -> bool:
    user_input = readkey()
    # Quit handler
    if user_input == 'q':
        save_exit(point_list)
        return False
    # Print new point prompt
    elif user_input == '\n':
        clear()
        new_point(point_list, port)
    # Enter delete mode
    elif user_input == 'x':
        console.print('Delete which point number?: ', end='')
        try:
            point_num = int(input())
        except ValueError as e:
            # Cancel if no value was input
            return True
        if point_num == 0:
            # Cancel if user entered 0
            return True
        
        point_list.pop(point_num - 1)
    
    return True

def render(point_list) -> str:
    buffer = "\n"
    header = "GPS Logging App"
    # Print header
    buffer += f"[bold red]{header: ^{WIDTH}}\n\n[cyan]"

    buffer += f"{'[Press q to Save and Exit]   [Press x to Enter Delete Mode]   [Press ENTER to Log a New Point]': ^{WIDTH}}\n\n"
    
    buffer += "\n Points Recorded This Run:\n\n"
    # Get longest name for neat buffer formatting
    if len(point_list) > 0:
        longest_name = max([len(point['name']) for point in point_list])
    else:
        longest_name = 0
    
    formatted_p_l = "\n".join([f"[white]{idx+1}. Name: [green]{point['name']: <{longest_name}}  [white]Latitude: [green]{point['lat']}   [white]Longitude: [green]{point['lon']}"
                               for idx, point in enumerate(point_list)])

    buffer += f"{formatted_p_l}"


    return buffer


def new_point(point_list : list, port) -> None:
    if not port:
        console.print("[bold red] COULDN'T CONNECT TO GPS, PRESS ANY KEY TO CONTINUE")
        port = get_port()
        readkey()
        return

    console.print("[bold yellow] Creating New Point\n")
    console.print("     Note: type [italic][bold]exit[/bold][/italic] and press enter to cancel creating a new point.\n")

    gnrmc = ""
    gnrmc_list = []
    # Iterate until full length GNRMC message has been obtained
    while len(gnrmc) < 41 or (not DEBUG and gnrmc_list[2] == 'V'):
        raw_lines = port.read(216).decode().split('\n')
        for line in raw_lines:
            if 'GNRMC' in line:
                gnrmc = line
                break
        gnrmc_list = gnrmc.split(',')

    # Get position info
    lat = gnrmc_list[3]
    lat_hem = gnrmc_list[4]

    lon = gnrmc_list[5]
    lon_hem = gnrmc_list[6]

    #console.print(gnrmc_list)
    console.print(f" [green]Lat: {lat}{lat_hem}   Lon: {lon}{lon_hem}")
    console.print(" [cyan]Point Name:   ", end="")
    point_name = input()

    # Detect cancellation
    if point_name == 'exit':
        return
    
    point_list.append({'name': point_name, 'lat': lat + lat_hem, 'lon': lon + lon_hem})

def clear():
    if sys.platform.startswith('linux'):
        os.system('clear')
    else:
        os.system('cls')

def save_exit(point_list):

    if len(point_list) > 0:
        with open(OUTPUT_FILENAME, "w") as fp:
            writer = csv.DictWriter(fp, fieldnames=list(point_list[0].keys()))
            writer.writeheader() # Write header
            writer.writerows(point_list) # Write data


def get_port():
    
    ports = comports()
    
    if len(ports) > 0:
        port_name = str(ports[0]).split(' ')[0]
        port = serial.Serial(port_name, baudrate=57600, timeout=3)
        return port

    return None
    
if __name__ == "__main__":
    main()
