import numpy as np

dx = 250 # steps
vi = 0 # steps per sec
vmax = 100 # steps per sec
acc = 200 # steps per sec^2
iacc = (vmax*vmax)/(2*acc)
print(iacc)
print("--------")
for i in range(dx):
    print(vi)
    if i < iacc:
        vi = np.sqrt( 2*acc *i)
    elif i > dx - iacc:
        vi = np.sqrt(2*acc*(dx-i))
    else:
        vi = vmax