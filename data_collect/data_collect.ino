// TODO: Check Serial.availableForWrite() before any writes?

// Just while testing, generates some dummy data:
#define NUM_SAMPLE_DATA 5

// Random number we expect for a valid request for data, and the default which indicates nothing was received
#define NO_REQUEST_RECEIVED 'N'
#define START_LOGGER        'S'
#define STOP_LOGGER         'T'
#define FETCH_DATA          'F'

// How often this program should check for requests received over serial
#define DATA_REQUEST_CHECK_FREQUENCY_MS 4000
// Number of fields in the data struct. Update this to send more values than two. If switching from ints to some other
// type, though, more work may be needed
#define NUM_DATA_FIELDS 2

 // We make three sensors, as an example.
#define NUM_SENSORS 3

// Enable/Disable error messages
#define DEBUG 0

struct data_point_struct {
  int header;
  int data;
};
typedef struct data_point_struct data_point;

struct sensor_struct {
  bool started;
  data_point *dp_arr;
  unsigned int dps_assigned; // The number of data points assigned (allocated is always + 1 of this)
};
typedef struct sensor_struct sensor;

sensor sensors[NUM_SENSORS];
unsigned int current_sensor = 0;

// Ideally wanted this to stop running anything else and just continually print an error message with a delay, but so
// far that's not working. Even a while(1) loop doesn't trap the program here, and loop() continues to run.
void error(const char errorMsg[]) {
  // Another possible approach that may or may not work?
  // sleep.enable();
  // set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  // sleep_cpu();
  // https://thekurks.net/blog/2018/1/24/guide-to-arduino-sleep-mode#anchor-link-3=
  if (DEBUG) Serial.println(errorMsg);
}

// Add data point to preexisting free spot in dp_arr, then allocate space for a new one
void append_data_point(unsigned int sensor_index, data_point * dp) {
  // dps_assigned always matches the next available spot in the array
  unsigned int * arr_index = &sensors[sensor_index].dps_assigned;
  sensors[sensor_index].dp_arr[*arr_index] = *dp;
  ++*arr_index; // Increment the number of data points assigned

  // Allocate space for new data point
  data_point *dp_arr_new;
                                                                   // used to do sizeof(dp) here instead, which seems to've been the major cause of problems for things being the wrong values :o
  dp_arr_new = (data_point*) realloc(sensors[sensor_index].dp_arr, sizeof(data_point) * (*arr_index + 1) );
  if (!dp_arr_new) {
    error("Error adding additional data (realloc)");
  }
  // If it didn't fail, overwrite the original pointer
  else sensors[sensor_index].dp_arr = dp_arr_new;
}

char serial_read_char() {

  // The default value which we check for
  char in = NO_REQUEST_RECEIVED;
  if (Serial.available() > 0) {
    char tmp[1];
    Serial.readBytes(tmp, 1);
    in = tmp[0];
  }
  return in;
}

data_point* malloc_dp() {
  data_point* dp = (data_point*) malloc(sizeof(data_point));
  if (!dp) error("Error allocating memory (malloc)");
  return dp;
}

// Initializes the array of data points for a sensor. Clears sensor data if there's already data present
void init_sensor_data(unsigned int sensor_index) {
  // If dp_arr already exists, free it
  if (sensors[sensor_index].dp_arr) free(sensors[sensor_index].dp_arr);

  // Allocate space for one element
  sensors[sensor_index].dp_arr = malloc_dp();
}

// Creates an array of sensors
void create_sensor(unsigned int sensor_index) {

  // Temperature, index 0  |  Humidity, index 1  |  Infrared, index 2
  sensor s;
  s.dps_assigned = 0;
  s.started = true;
  sensors[sensor_index] = s;
  init_sensor_data(sensor_index); 
}

// Add NUM_SAMPLE_DATA data points to the sensor
// Just used for simulation while there's no real sensors to generate data
void add_sample_data(unsigned int sensor_index) {
  for (int j = 0; j < NUM_SAMPLE_DATA; j++) {
    data_point *dp = malloc_dp();
    // Using easily recognizable repeated values just while testing, to see if they appear correctly on the client side
    dp->header = 1; // TEST this with higher numbers.
    dp->data = 29; // TEST
    //dp->header = i+j; // Just some arbitrary calculations to create variation in the data
    //dp->data = i+j+10;
    append_data_point(sensor_index, dp);
  }
}

// Serializes data for the current sensor and then sends it over serial
void send_data(unsigned int sensor_index) {

  // Loop through data points, only sends data if we have any
  for (unsigned int i = 0; i < sensors[sensor_index].dps_assigned; i++) {

    // Would like to loop through fields in the data point to make it more flexible, but it seems they can only be accesssed by name, not by index?
    for (int j = 0; j < NUM_DATA_FIELDS; j++) {
      if (j == 0) Serial.print(sensors[sensor_index].dp_arr[i].header);
      else Serial.print(sensors[sensor_index].dp_arr[i].data);
      // Update: This shouldn't be needed, but unfortunately numbers of multiple digits are split up on the receiving side, 
      // even if they should fit in a byte (0-255 if unsigned) so I put this back in
      if (j != NUM_DATA_FIELDS-1) Serial.print("|"); // Separator between each value in a Data Point. Don't add a separator after the last value
    }
    Serial.print('@'); // Separator between Data Points, not after the last one
  }

  // Newline means the series of data points have been sent. Ensure it's never part of the data itself.
  // Here they instead use a null byte: https://forum.arduino.cc/index.php?topic=96037.0
  Serial.println();

  // Clear up data points once they've been sent, so they don't accumulate.
  // This is not ideal if every data point is crucial though, in case of transmission errors.
  init_sensor_data(current_sensor);
}

void setup() {

  Serial.begin(9600);     // Baud rate
  delay(2000);

  for (unsigned int i = 0; i < NUM_SENSORS; i++) {
    create_sensor(i);        // Creates the specified number of Sensors with no data
    add_sample_data(i);      // Adds some sample data
  }
}

// Continually check whether data is requested, and send the next one when it is.
// Send it sequentially, and if there's no new data to send, send some code/signal signifying that.
void loop() {
  // It receives the ASCII value in (7 is 55 in the ASCII table), but assigning it to a char converts it automatically

  char in = serial_read_char();
  switch(in) {
    case NO_REQUEST_RECEIVED:
      error("No request received yet"); break;
    // 1-3 are for sensor switching
    case '1':
    case '2':
    case '3':
      error(in); // TESTING
      // Convert to int for array indexing. Numbers sent are chars and not zero indexed.
      current_sensor = in - 1; 
      break;
    // Start/Stop logger should start/stop whether current_sensor is read from - or in testing whether new data is generated continuously
    case START_LOGGER:
      sensors[current_sensor].started = true;
      error("TEST_START");
      break;
    case STOP_LOGGER:
      sensors[current_sensor].started = false;
      error("TEST_STOP");
      break;
    case FETCH_DATA:
      send_data(current_sensor);
      error("TEST_FETCH");
      break;
    default:
      error("Invalid request");
      break; 
  }
  delay(DATA_REQUEST_CHECK_FREQUENCY_MS);
}
