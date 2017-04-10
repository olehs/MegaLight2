#include "ExpressionEvaluator.h"

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

enum {ASSOC_NONE=0, ASSOC_LEFT, ASSOC_RIGHT};

int eval_and(int a1, int a2) { return a1 & a2 ? 1 : 0; }
int eval_or(int a1, int a2) { return a1 | a2 ? 1 : 0; }
int eval_not(int a1, int a2) { return a1 ? 0 : 1; }
int eval_equal(int a1, int a2) { return a1 == a2 ? 1 : 0; }
int eval_greater(int a1, int a2) { return a1 > a2 ? 1 : 0; }
int eval_less(int a1, int a2) { return a1 < a2 ? 1 : 0; }
int eval_ge(int a1, int a2) { return a1 >= a2 ? 1 : 0; }
int eval_le(int a1, int a2) { return a1 <= a2 ? 1 : 0; }

operator_type operators[] = {
    {'&', 4, ASSOC_LEFT,  0, eval_and},
    {'|', 3, ASSOC_LEFT,  0, eval_or},
    {'!', 8, ASSOC_RIGHT, 1, eval_not},
    {'=', 5, ASSOC_LEFT,  0, eval_equal},
    {'>', 6, ASSOC_LEFT,  0, eval_greater},
    {'<', 6, ASSOC_LEFT,  0, eval_less},
    {'g', 6, ASSOC_LEFT,  0, eval_ge},
    {'l', 6, ASSOC_LEFT,  0, eval_le},
    {'(', 0, ASSOC_NONE,  0, NULL},
    {')', 0, ASSOC_NONE,  0, NULL}
};

ExpressionEvaluator::ExpressionEvaluator(TokenEvaluator *ev)
    : tokenEvaluator(ev)
{
}

operator_type *ExpressionEvaluator::getop(char ch) {
    for(int i = 0; i < sizeof(operators) / sizeof(operator_type); ++i) {
        if(operators[i].op == ch)
            return &operators[i];
    }
    return NULL;
}


void ExpressionEvaluator::push_opstack(operator_type *op)
{
    if(nopstack > MAXOPSTACK - 1) {
//        Serial.println("ERROR: Operator stack overflow");
        return;
    }
    opstack[nopstack++] = op;
}

operator_type *ExpressionEvaluator::pop_opstack()
{
    if(!nopstack) {
//        Serial.println("ERROR: Operator stack empty");
        return NULL;
    }
    return opstack[--nopstack];
}

void ExpressionEvaluator::push_numstack(int num)
{
    if(nnumstack > MAXNUMSTACK - 1) {
//        Serial.println("ERROR: Number stack overflow");
        return;
    }
    numstack[nnumstack++] = num;
}

int ExpressionEvaluator::pop_numstack()
{
    if(!nnumstack) {
//        Serial.println("ERROR: Number stack empty");
        return 0;
    }
    return numstack[--nnumstack];
}


void ExpressionEvaluator::shunt_op(operator_type *op)
{
    operator_type *pop;
    int n1, n2;

    if(op->op == '(') {
        push_opstack(op);
        return;

    } else if(op->op == ')') {
        while(nopstack > 0 && opstack[nopstack-1]->op != '(') {
            pop = pop_opstack();
            n1 = pop_numstack();

            if(pop->unary)
                push_numstack(pop->eval(n1, 0));
            else {
                n2=pop_numstack();
                push_numstack(pop->eval(n2, n1));
            }
        }

        if(!(pop = pop_opstack()) || pop->op != '(') {
//            Serial.println("ERROR: Stack error. No matching \'(\'");
            return;
        }
        return;
    }

    if(op->assoc == ASSOC_RIGHT) {
        while(nopstack && op->prec<opstack[nopstack-1]->prec) {
            pop = pop_opstack();
            n1 = pop_numstack();
            if(pop->unary)
                push_numstack(pop->eval(n1, 0));
            else {
                n2 = pop_numstack();
                push_numstack(pop->eval(n2, n1));
            }
        }
    } else {
        while(nopstack && op->prec<=opstack[nopstack-1]->prec) {
            pop = pop_opstack();
            n1 = pop_numstack();
            if(pop->unary)
                push_numstack(pop->eval(n1, 0));
            else {
                n2 = pop_numstack();
                push_numstack(pop->eval(n2, n1));
            }
        }
    }
    push_opstack(op);
}

int ExpressionEvaluator::istoken_char(char c) {
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || isdigit(c) || (c == '_'))
        return 1;
    else
        return 0;
}

int ExpressionEvaluator::eval(const char *expression)
{
    nopstack = 0;
    nnumstack = 0;

    char *tstart = 0;
    operator_type startop = {'X', ASSOC_NONE, 0, 0};  /* Dummy operator to mark start */
    operator_type *op = 0;
    int n1, n2;
    operator_type *lastop = &startop;

    for (char *expr = (char*)expression; *expr; ++expr) {
        if (!tstart) {

            if ((op = getop(*expr))) {
                if (lastop && (lastop == &startop || lastop->op != ')')) {
                    if ((op->op != '(') && !op->unary) {
                       Serial.print("ERROR: Illegal use of binary operator "); Serial.println(op->op);
                        return EVAL_FAILURE;
                    }
                }

                shunt_op(op);
                lastop = op;
            } else if (istoken_char(*expr))
                tstart = expr;
            else if (!isspace(*expr)) {
               Serial.println("ERROR: Syntax error");
                return EVAL_FAILURE;
            }
        } else {
            if (isspace(*expr)) {
                push_numstack((*tokenEvaluator)(tstart));
                tstart = 0;
                lastop = 0;
            } else if ((op=getop(*expr))) {
                push_numstack((*tokenEvaluator)(tstart));
                tstart = 0;
                shunt_op(op);
                lastop = op;
            } else if (!istoken_char(*expr) ) {
               Serial.println("ERROR: Syntax error");
                return EVAL_FAILURE;
            }
        }
    }
    if(tstart)
        push_numstack((*tokenEvaluator)(tstart));

    while(nopstack) {
        op = pop_opstack();
        n1 = pop_numstack();
        if(op->unary)
            push_numstack(op->eval(n1, 0));
        else {
            n2 = pop_numstack();
            push_numstack(op->eval(n2, n1));
        }
    }

    if(nnumstack != 1) {
       Serial.print("ERROR: Number stack has "); Serial.print(nnumstack); Serial.println(" elements after evaluation. Should be 1.");
        return EVAL_FAILURE;
    }

    return numstack[0];
}
