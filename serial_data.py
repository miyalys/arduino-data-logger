#! /usr/bin/env python

# TODO: Handle if the serial connection is involuntarily closed, attempting to reconnect?

import signal, sys, time, serial, os, itertools
from threading import Thread

# CONFIG:
ARDUINO_DEVICE_PATH='/dev/ttyACM0'
DATA_REQUEST_CHECK_FREQUENCY_S = 3
# Sending a single char. Little data but still allows for sending a lot of different commands to the Arduino.
# 256 if ASCII, and far more if unicode.
# Serial.write can't take a utf-8 string, so converting to a byte array
FETCH_DATA_CODE = bytes("F", "ascii")
START_LOGGER_CODE = bytes("S", "ascii")
STOP_LOGGER_CODE = bytes("T", "ascii")
debug = False

# GLOBAL VARIABLES
logger_state = False
sensors = []
current_sensor = 0 # An index into the Sensors list
debug_log = ""

# HELPERS

class Sensor():

  def __init__(self, name):
    self.name = name
    self.data = ""

# These arguments are only taken because signal.signal below passes two arguments, though I don't use them for anything
# ...and it seems to make sense to reuse this for exiting from the menu, and not just Ctrl-c
def exit(throwaway1=1, throwaway2=2):
  global debug

  print('Exiting!')
  debug = False
  arduino_serial.close()
  sys.exit(0)


# Setup closing the connection and quit the program if the user sends the sigint signal (Ctrl-c in Unix like systems)
signal.signal(signal.SIGINT, exit)

# Write to file
def file_write(data):
  # a: write append. without b it's assumed to be text rather than random binary data.
  # with closes files automatically after the scope is left
  with open('data.log', 'a') as f:
    f.write(data)

def menu():
  os.system('cls' if os.name == 'nt' else 'clear')

  print("Logger is currently: " + ("Started" if logger_state else "Stopped"))
  print("Current sensor: " + sensors[current_sensor].name + "\n")

  print("1: Start/Stop logger")
  print("2: Next sensor")
  print("3: Fetch data")
  print("4: Print data")
  print("5: Quit" + "\n")

def start_stop_logger():

  global logger_state

  msg = START_LOGGER_CODE if logger_state else STOP_LOGGER_CODE

  # send the character to the device.
  arduino_serial.write(msg)

  # Should I wait for a specific response before locally setting the logger_state to enabled?
  logger_state = not logger_state

# TODO: Need to tell the Arduino which sensor, and then we should read into its data specifically
# Also change the data format/protocol?
def read_serial():
  global arduino_serial

  read = []
  # in_waiting: Get the number of bytes in the input buffer
  if (arduino_serial.in_waiting > 0):
    while True:
      # Currently reading one byte at a time, that way the last byte read will always be a newline, and not half-way into a data point.
      read_raw = arduino_serial.read(1) # number of bytes to read.
      if read_raw == b'\r': break # Don't add the newline to the data and stop the loop when it's found. \r\n is what it sends, since \r comes first we check for that
      else: read.append(read_raw.decode("ascii"))
      time.sleep(0.1)
      #print("Still in loop")
    #print("Read: ); print(read)
    pressEnterToContinue("Finished reading data from the current sensor")
  else:
    pressEnterToContinue("No data is currently available")

  return read

def pressEnterToContinue(msg):
  if (msg): print(msg)
  print() # Newline
  input("Press enter to continue")  # TODO: Just while testing, improve this later

# Continuously prints anything received over the serial connection
def debug_log():
  global debug_log
  while (debug):
    debug_log = read_serial()
    print(debug_log)
    time.sleep(DATA_REQUEST_CHECK_FREQUENCY_S)

def parse_data(raw_data):
  data = raw_data[0:-1] # remove dangling, trailing @
  data_point_separator = "@"
  value_separator="|"

  # Remove dp separators and turn the result into a list of lists, where each list is a data point
  dp_separators_removed = [list(y) for x, y in itertools.groupby(data, lambda z: z == data_point_separator) if not x]
  #return dp_separators_removed # TESTING

  # Concat the numbers on the same side of the separators back together
  cleaned_dps = []
  cleaned_dp = []
  for dp in dp_separators_removed: # Loop through each dp (list) from the list of all dps
    #return dp # TESTING
    value = ""
    for elem in dp: # Loop through each character in a single dp
      if (elem != value_separator):
        value+=elem
      else: # If a separator is found add the value to the cleaned dp, and convert it from string to integer in the process
        cleaned_dp.append( int(value) )
        value = "" # empty data point value (a single field within a dp) to begin constructing a new one
    cleaned_dp.append( int(value) ) # If the end of a dp is reached append whatever's so far in value onto the list as well
    cleaned_dps.append(tuple(cleaned_dp)) # add the cleaned dp to the list of all dps, and convert it to a tuple in the process
    cleaned_dp = [] # empty list to make space for a new cleaned_dp

  return cleaned_dps

def init():
  global current_sensor, arduino_serial, debug_thread, debug

  # THE PROGRAM ITSELF

  sensors.append(Sensor('Temperature'))
  sensors.append(Sensor('Humidity'))
  sensors.append(Sensor('Infrared'))

  arduino_serial = serial.Serial(
      port=ARDUINO_DEVICE_PATH,
      baudrate=9600,
      parity=serial.PARITY_ODD, # TODO: Look into these three, what they should be and why
      stopbits=serial.STOPBITS_TWO,
      bytesize=serial.SEVENBITS
  )
  # Fail if the serial connection hasn't been opened.
  if not arduino_serial.isOpen():
    print(f'Error: {ARDUINO_DEVICE_PATH} cannot be found. Is the Arduino connected?')
    exit()


  debug_thread = Thread(target = debug_log)
  debug_thread.start()

  while(True):
    menu()
    if debug:
      print("\nDebug log:")
    user_read_raw = input()
    user_read = "E" if user_read_raw == "" else int(user_read_raw) # Error handling a user just pressing enter, as int conversion on an empty string will fail. E is just short for "error"

    # No Switch in Python
    if (user_read == 1): start_stop_logger()
    elif (user_read == 2): current_sensor = (current_sensor+1 if current_sensor < len(sensors)-1 else 0)
    elif (user_read == 3):
      arduino_serial.write(FETCH_DATA_CODE)
      resp = read_serial()
      if resp:
        #print(resp) # debug
        sensors[current_sensor].data = parse_data(resp)
      # Test first, then once it works append it to a file
      # file_write(read)
    elif (user_read == 4):
      #pressEnterToContinue("Data: ") + sensors[current_sensor].data)
      print("Data: ")
      pressEnterToContinue(sensors[current_sensor].data)
    elif (user_read == 5):
      debug = False
      exit()
    else: pressEnterToContinue('Invalid input - try again')


# Start the program
init()
