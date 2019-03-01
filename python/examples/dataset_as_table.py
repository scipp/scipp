#| 
# Imports
from dataset import *
import numpy as np
# Uncomment for jupyter
#%matplotlib inline
#--------------------

#| 
# Create empty dataset
table = Dataset()
#---------------------

#| 
# Add columns
table[Coord.RowLabel] = ([Dim.Row], ['a', 'bb', 'ccc', 'dddd'])
table[Data.Value, "col1"] = ([Dim.Row], [3,2,1,0])
table[Data.Value, "col2"] = ([Dim.Row], np.arange(4, dtype=np.float64))
table[Data.Value, "sum"] = ([Dim.Row], (4,))
#---------------------------------------------

#| 
#Do something for each column (here: sum)
for col in table:
    if not col.is_coord and col.name is not "sum":
        table[Data.Value, "sum"] += col
#-----------------------------------------------------

#| 
# Append tables (append rows of second table to first)
table = concatenate(table, table, Dim.Row)
#------------------------------------------------------

#|
# Append tables sections (e.g., to remove rows from the middle)
table = concatenate(table[Dim.Row, 0:2], table[Dim.Row, 5:7], Dim.Row)
#----------------------------------------------------------------

#|
# Sort by column
table = sort(table, Data.Value, "col1")
# ... or another one
table = sort(table, Coord.RowLabel)
#-------------------------------------

#|
# Do something for each row (here: cumulative sum)
for i in range(1, len(table[Coord.RowLabel])):
    table[Dim.Row, i] += table[Dim.Row, i-1]

# Apply numpy function to column, store result as a new column
table[Data.Value, "exp1"] = ([Dim.Row], np.exp(table[Data.Value, "col1"]))
# ... or as an existing column
table[Data.Value, "exp1"] = np.sin(table[Data.Value, "exp1"])
#---------------------------------------------

#|
# Remove column
del table[Data.Value, "exp1"]
#---------------------------------

#|
# Arithmetics with tables (here: add two tables)
table += table
xr_ds = as_xarray(table)
xr_ds['Value:col1'].plot()
#----------------------------------------------------------
