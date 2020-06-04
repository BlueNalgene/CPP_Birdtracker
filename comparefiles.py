import numpy as np

data_cpp = "./test.csv"
data_pyt = "./test_py.csv"

cppd = np.genfromtxt(data_cpp, delimiter=',')
pytd = np.genfromtxt(data_pyt, delimiter=',')

# only use data up to seg fault
max_cpp_frame = np.max(cppd[:, 0])
print("cpp max frame reached:", max_cpp_frame)
pytd = pytd[np.where(pytd[:, 0] < (max_cpp_frame + 1))]

print("cpp data shape", np.shape(cppd))
print("pyt data shape", np.shape(pytd))
