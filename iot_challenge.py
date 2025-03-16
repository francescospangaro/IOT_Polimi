import math
import numpy as np

# Constants
b = 2000        # bits
Ec = 50         # nj/bit
k = 1           # nj/bit*m2
sensor_coordinates = [
    (1,2),(10,3),
    (4,8),(15,7),
    (6,1),(9,12),
    (14,4),(3,10),
    (7,7),(12,14)
]


# Computation parameters
PRECISION = 0.01
MAX_X_SEARCH_SPACE = 20.0
MAX_Y_SEARCH_SPACE = 20.0
MIN_X_SEARCH_SPACE = 0.0
MIN_Y_SEARCH_SPACE = 0.0

# Defining the search space
(X,Y) = (np.arange(MIN_X_SEARCH_SPACE,MAX_X_SEARCH_SPACE,PRECISION),np.arange(MIN_Y_SEARCH_SPACE,MAX_Y_SEARCH_SPACE,PRECISION))

# Function that compute the power consumed for transmission
#   - d: Transmission distance
def Etx(d):
  return k*math.pow(d,2)

# Function that compute, given the sink coordinates, the sensor that consume the most power
#   - xs: X coordinate of the sink
#   - ys: Y coordinate of the sink
def worst_case_sensor(xs, ys):
  max_power_consumption = float("-inf")

  # Cycling over the sensors coordinates
  for coords in sensor_coordinates:
    # Computing the distance between the current sensor and the sink
    distance = math.sqrt(math.pow(xs-coords[0],2)+math.pow(ys-coords[1],2))

    # Computing the power consumption of the current sensor during the duty cycle (Circuitry activation + Transmission)
    power_consumption = (Ec + Etx(distance))*b

    # Storing the highest power consumption obtained with the sink node at the coordinates (xs, ys)
    if power_consumption > max_power_consumption:
      max_power_consumption = power_consumption

  # Returning the max power consumption computed
  return max_power_consumption


power_consumptions = []
for xs in X:
  for ys in Y:
    power_consumptions.append((worst_case_sensor(xs,ys),(xs,ys)))

# Computing the smallest power consumption among the maximum power consumption for each sink position.
# The power computed will be associated with the optimal position of the sink node
optimal_pos = min(power_consumptions, key=lambda x: x[0])
print(f"Sink optimal position is: {optimal_pos[1]}, with power consumption: {optimal_pos[0]}")
