#include "straightline/slp.h"

#include <iostream>

namespace A {
    int A::CompoundStm::MaxArgs() const {
        // put your code here (lab1).
        int l = stm1->MaxArgs();
        int r = stm2->MaxArgs();
        return (l > r) ? l : r;
    }

    Table *A::CompoundStm::Interp(Table *t) const {
        // put your code here (lab1).
        t = stm1->Interp(t);
        t = stm2->Interp(t);
        return t;
    }

    int A::AssignStm::MaxArgs() const {
        // put your code here (lab1).
        return exp->MaxArgs();
    }

    Table *A::AssignStm::Interp(Table *t) const {
        // put your code here (lab1).
        IntAndTable *iat = exp->Interp(t);
        t = iat->t;
        return t->Update(id, iat->i);
    }

    int A::PrintStm::MaxArgs() const {
        // put your code here (lab1).
        int l = exps->MaxArgs();
        int r = exps->NumExps();
        return (l > r) ? l : r;
    }

    Table *A::PrintStm::Interp(Table *t) const {
        // put your code here (lab1).
        IntAndTable *iat = exps->Interp(t);
        // TODO: print every values in 'exps', now in 'Interp()'
        return iat->t;
    }


    int Table::Lookup(const std::string &key) const {
        if (id == key) {
            return value;
        } else if (tail != nullptr) {
            return tail->Lookup(key);
        } else {
            assert(false);
        }
    }

    Table *Table::Update(const std::string &key, int val) const {
        return new Table(key, val, this);
    }

    int IdExp::MaxArgs() const {
        return 0;
    }

    IntAndTable *IdExp::Interp(Table *t) const {
        int val = t->Lookup(id);
        return new IntAndTable(val, t);
    }

    int NumExp::MaxArgs() const {
        return 0;
    }

    IntAndTable *NumExp::Interp(Table *t) const {
        return new IntAndTable(num, t);
    }

    int OpExp::MaxArgs() const {
        int l = left->MaxArgs();
        int r = right->MaxArgs();
        return (l > r) ? l : r;
    }

    IntAndTable *OpExp::Interp(Table *t) const {
        IntAndTable *iat1 = left->Interp(t);
        IntAndTable *iat2 = right->Interp(iat1->t);
        int result = 0;
        switch (oper) {
            case PLUS:
                result = iat1->i + iat2->i;
                break;
            case MINUS:
                result = iat1->i - iat2->i;
                break;
            case TIMES:
                result = iat1->i * iat2->i;
                break;
            case DIV:
                result = iat1->i / iat2->i;
                break;
            default:
                break;
        }
        return new IntAndTable(result, iat2->t);
    }

    int EseqExp::MaxArgs() const {
        int l = stm->MaxArgs();
        int r = exp->MaxArgs();
        return (l > r) ? l : r;
    }

    IntAndTable *EseqExp::Interp(Table *t) const {
        t = stm->Interp(t);
        return exp->Interp(t);
    }


    int PairExpList::MaxArgs() const {
        int l = exp->MaxArgs();
        int r = tail->MaxArgs();
        return (l > r) ? l : r;
    }

    int PairExpList::NumExps() const {
        return (tail->NumExps() + 1);
    }

    IntAndTable *PairExpList::Interp(Table *t) const {
        IntAndTable *iat = exp->Interp(t);
        std::cout << iat->i << ' ';
        return tail->Interp(iat->t);
    }

    int LastExpList::MaxArgs() const {
        return exp->MaxArgs();
    }

    int LastExpList::NumExps() const {
        return 1;
    }

    IntAndTable *LastExpList::Interp(Table *t) const {
        IntAndTable *iat = exp->Interp(t);
        std::cout << iat->i << '\n';
        return iat;
    }

}  // namespace A
