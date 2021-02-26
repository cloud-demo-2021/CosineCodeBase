void getNoOfLevels(int* L, double M_B, int T, double data);
void getNoOfLevelsAvgCase(int* L, double M_B, int T, double data);
void getNoOfLevelsWacky(int* L, double M_B, int T, double data, int C);
void getX(double* X, int T, int K, int Z);
void getY(int* Y, double* M_FP, double M_F, double M_F_HI, double M_F_LO, double X, int T, int L, double data);
void getM_FP(double* M_FP, int T, int L, int Y, double M_B, double M_F_LO);
void getM_BF(double* M_BF, double M_F, double M_FP);
int validateFilterMemoryLevels(double M_F, double M_F_HI, double M_F_LO);
void set_M_B(double* M_F, double* M_B, double M, double M_F_HI, double M_F_LO);
void set_M_F(double* M_F, double* M_B, double M, double M_F_HI, double M_F_LO);
void getM_F_LO(double* M_F_LO, double *M_B, double M, int T, double data);







