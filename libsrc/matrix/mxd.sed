/include.*<float.h>/b
s/float/double/g
s/mxs_real/double/g
/^typedef.*double.*double;/d
s/mxs_/mxds_/g
s/mxds_ang/mxs_ang/g
s/mx_/mxd_/g
s/#define MX_VERSION/#define MXD_VERSION/g
s/matrix\.h/matrixd.h/g
s/matrixs\.h/matrixds.h/g
s/mxsym\.h/mxsymd.h/g
s/__MATRIX_H/__MATRIXD_H/g
s/__MATRIXS_H/__MATRIXDS_H/g
s/__MXSYM_H/__MXSYMD_H/g
