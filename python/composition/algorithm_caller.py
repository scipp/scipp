from composition_exports import *

print(rebin(5));

# We would like this, but it fails, is there a way in combination with overloads?
# args = {'histogram':5.0, 'scale':4, 'history':None}
args = {'histogram':5.0, 'scale':4}
print(convertUnits(**args));
args2 = [5.0, 4.0]
print(convertUnits(*args2));
