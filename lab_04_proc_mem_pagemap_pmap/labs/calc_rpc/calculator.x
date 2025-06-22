const ADD = 0;
const SUB = 1;
const MUL = 2;
const DIV = 3;

struct CALCULATOR {
    int op;
    double arg1;
    double arg2;
    int number;
    double result;
};

program CALCBAKERY_PROG {
    version CALCULATOR_VER {
        int CALC_GET_INDEX(void) = 1;
        double CALC_SERV(struct CALCULATOR) = 2;
    } = 1;
} = 0x20000001;

