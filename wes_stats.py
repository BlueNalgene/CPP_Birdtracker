import h5py
from matplotlib import pyplot as plt
import numpy as np
from sklearn.model_selection import train_test_split
from sklearn.discriminant_analysis import LinearDiscriminantAnalysis as LDA
from sklearn.metrics import confusion_matrix
from sklearn.metrics import accuracy_score
from sklearn.preprocessing import StandardScaler

# Column Titles to values
FRM0 = 0      # frame at n-0
XXX0 = 1      # contour x at n-0
YYY0 = 2      # contour y at n-0
RAD0 = 3      # contour radius at n-0
LAP0 = 4      # contour average laplacian at n-0
PRM0 = 5      # contour perimeter at n-0
FRM1 = 6      # frame at n-1 .......
XXX1 = 7
YYY1 = 8
RAD1 = 9
LAP1 = 10
PRM1 = 11
FRM2 = 12
XXX2 = 13
YYY2 = 14
RAD2 = 15
LAP2 = 16
PRM2 = 17
DST01 = 18    # Distance from contours 0 to 1
DST12 = 19
DIR01 = 20    # Direction from contour 0 to 1
DIR12 = 21
SDST = 22     # Distance score
SDIR = 23     # Direction score
SRAD = 24     # Radius variance
SLAP = 25     # average Laplacian variance
SPRM = 26     # peimeter variance
ADST = 27     # averaged distance
ADIR = 28     # averaged direction
ARAD = 29     # average radius
EXXX = 30     # ellipse x center
EYYY = 31     # ellipse y center
EAAA = 32     # ellipse major axis
EBBB = 33     # ellipse minor axis
ETHT = 34     # ellipse angle theta
ISRH = 35     # ISR hit or miss
ELLX = 36     # x of target ellipse point
ELLY = 37     # y of target ellipse point
MAXD = 38     # max distance to edge of frame
STKS = 39     # number of potential hits on path to ellipse edge
SRAD = 40     # number of potential hits with similar radii
NDST = 41     # number of potential hits with similar speed
NDIR = 42     # number of potential hits with similar direction






def plot_scikit_lda(X, iny, title):
	ax = plt.subplot(111)
	hits = X[np.where(iny == 1)]
	miss = X[np.where(iny == 0)]
	label = ("hits", "miss")
	color = ('blue', 'red')
	plt.scatter(x=miss[:,0], y=miss[:,1] * -1, color=color[1], alpha=0.5, label=label[1])
	plt.scatter(x=hits[:,0], y=hits[:,1] * -1, color=color[0], alpha=0.5, label=label[0])
	plt.xlabel('LD1')
	plt.ylabel('LD2')
	leg = plt.legend(loc='upper right', fancybox=True)
	leg.get_frame().set_alpha(0.5)
	plt.title(title)
	# hide axis ticks
	plt.tick_params(axis="both", which="both", bottom="off", top="off", labelbottom="on", left="off", right="off", labelleft="on")
	# remove axis spines
	ax.spines["top"].set_visible(False)
	ax.spines["right"].set_visible(False)
	ax.spines["bottom"].set_visible(False)
	ax.spines["left"].set_visible(False)
	plt.grid()
	plt.tight_layout
	plt.show()






#indata = np.loadtxt("./data/output.csv", delimiter=',')
#indata = np.loadtxt("./data/testout.csv", delimiter=',')
with h5py.File("temp.hdf5", 'r') as indata:
	print("input data shape", indata["data"].shape)
	inx = np.column_stack((indata["data"][:, SDST:SPRM+1], indata["data"][:, ETHT], indata["data"][:, MAXD:STKS+1]))
	iny = indata["data"][:, ISRH]

	x_train, x_test, y_train, y_test = train_test_split(inx, iny, test_size=0.2, random_state=0)

sc = StandardScaler()

n_train = x_train.shape[0]
batch_size = 100000

index = 0
while index < n_train:
	print("ntrain index =", int(index/batch_size))
	partial_size = min(batch_size, n_train - index)
	partial_x = x_train[index:index+partial_size]
	#partial_y = y_train[index:index+partial_size]
	sc.partial_fit(partial_x)
	index += partial_size
print("x_train transform")
x_train = sc.fit_transform(x_train)


print("x_train shape:", x_train.shape)

lda = LDA(n_components=1)

index = 0
while index < n_train:
	print("lda fit index =", index/batch_size)
	partial_size = min(batch_size, n_train - index)
	partial_x = x_train[index:index+partial_size]
	partial_y = y_train[index:index+partial_size]
	_ = lda.fit_transform(partial_x, partial_y)
	index += partial_size
#x_train = lda.fit_transform(x_train, y_train)

y_pred = lda.predict(x_test)
x_test = lda.transform(x_test)

print(confusion_matrix(y_test, y_pred))
print("Accuracy", str(accuracy_score(y_test, y_pred)))



#plot_scikit_lda(x_train, y_train, title='Default LDA via scikit-learn')
