#ifndef EXPRESSIONEVALUATOR_H
#define EXPRESSIONEVALUATOR_H

#define MAXOPSTACK 64
#define MAXNUMSTACK 64

#define EVAL_FAILURE -1

class TokenEvaluator{
public:
    virtual int operator()( char *expr )=0;
};

template<typename F>
class GenericTokenEvaluator : public TokenEvaluator
{
    F* f;
public:
    GenericTokenEvaluator(F _f): f(_f) {}
    int operator()( char *expr )
    {
        return f(expr);
    }
};

template<class C>
class MemberFunctionTokenEvaluator : public TokenEvaluator
{
    typedef int (C::*memberf_pointer)(char*);
public:
    MemberFunctionTokenEvaluator() {}
    MemberFunctionTokenEvaluator(C* _obj, memberf_pointer _f):obj(_obj),f(_f) {}

    C* obj;
    memberf_pointer f;

    int operator()( char *expr )
    {
        return ((*obj).*f) (expr);
    }

};

struct operator_type {
  char op;
  int prec;
  int assoc;
  int unary;
  int (*eval)(int a1, int a2);
};

class ExpressionEvaluator
{
public:
    ExpressionEvaluator(TokenEvaluator *ev);
    int eval(const char *expression);
    static int istoken_char(char c);

private:

    operator_type *opstack[MAXOPSTACK];
    int numstack[MAXNUMSTACK];

    int nopstack;
    int nnumstack;

    TokenEvaluator *tokenEvaluator;

    operator_type *getop(char ch);
    void push_opstack(operator_type *op);
    operator_type *pop_opstack();
    void push_numstack(int num);
    int pop_numstack();
    void shunt_op(operator_type *op);
};

#endif // EXPRESSIONEVALUATOR_H
