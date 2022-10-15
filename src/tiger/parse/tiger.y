%filenames parser
%scanner tiger/lex/scanner.h
%baseclass-preinclude tiger/absyn/absyn.h

 /*
  * Please don't modify the lines above.
  */

%union {
  int ival;
  std::string* sval;
  sym::Symbol *sym;
  absyn::Exp *exp;
  absyn::ExpList *explist;
  absyn::Var *var;
  absyn::DecList *declist;
  absyn::Dec *dec;
  absyn::EFieldList *efieldlist;
  absyn::EField *efield;
  absyn::NameAndTyList *tydeclist;
  absyn::NameAndTy *tydec;
  absyn::FieldList *fieldlist;
  absyn::Field *field;
  absyn::FunDecList *fundeclist;
  absyn::FunDec *fundec;
  absyn::Ty *ty;
  }

%token <sym> ID
%token <sval> STRING
%token <ival> INT

%token
  COMMA COLON SEMICOLON LPAREN RPAREN LBRACK RBRACK
  LBRACE RBRACE DOT
  ASSIGN
  ARRAY IF THEN ELSE WHILE FOR TO DO LET IN END OF
  BREAK NIL
  FUNCTION VAR TYPE

 /* token priority */
 /* TODO: Put your lab3 code here */
%left OR
%left AND
%nonassoc EQ NEQ LT GT LE GE
%left PLUS MINUS
%left TIMES DIVIDE
%left UMINUS

%type <exp> exp expseq
%type <explist> actuals nonemptyactuals sequencing sequencing_exps
%type <var> lvalue one oneormore
%type <declist> decs decs_nonempty
%type <dec> decs_nonempty_s vardec
%type <efieldlist> rec rec_nonempty
%type <efield> rec_one
%type <tydeclist> tydec
%type <tydec> tydec_one
%type <fieldlist> tyfields tyfields_nonempty
%type <field> tyfield
%type <ty> ty
%type <fundeclist> fundec
%type <fundec> fundec_one

%start program

%%
program:
     exp  {absyn_tree_ = std::make_unique<absyn::AbsynTree>($1);};

lvalue:
     ID  {$$ = new absyn::SimpleVar(scanner_.GetTokPos(), $1);}
  |  oneormore  {$$ = $1;}
  ;

 /* TODO: Put your lab3 code here */
oneormore:
     one  {$$ = $1;}
  |  oneormore LBRACK exp RBRACK  {$$ = new absyn::SubscriptVar(scanner_.GetTokPos(), $1, $3);}
  |  oneormore DOT ID  {$$ = new absyn::FieldVar(scanner_.GetTokPos(), $1, $3);}
  ;
one:
     ID LBRACK exp RBRACK  {$$ = new absyn::SubscriptVar(scanner_.GetTokPos(), new absyn::SimpleVar(scanner_.GetTokPos(), $1), $3);}
  |  ID DOT ID  {$$ = new absyn::FieldVar(scanner_.GetTokPos(), new absyn::SimpleVar(scanner_.GetTokPos(), $1), $3);}
  ;
  
exp:
     INT  {$$ = new absyn::IntExp(scanner_.GetTokPos(), $1);}
  |  STRING  {$$ = new absyn::StringExp(scanner_.GetTokPos(), $1);}
  |  NIL  {$$ = new absyn::NilExp(scanner_.GetTokPos());}
  |  lvalue  {$$ = new absyn::VarExp(scanner_.GetTokPos(), $1);}
  |  lvalue ASSIGN exp  {$$ = new absyn::AssignExp(scanner_.GetTokPos(), $1, $3);}
  |  BREAK  {$$ = new absyn::BreakExp(scanner_.GetTokPos());}

  |  ID LBRACK exp RBRACK OF exp  {$$ = new absyn::ArrayExp(scanner_.GetTokPos(), $1, $3, $6);}

  |  LET decs IN expseq END  {$$ = new absyn::LetExp(scanner_.GetTokPos(), $2, $4);}

  |  LPAREN RPAREN  {$$ = new absyn::VoidExp(scanner_.GetTokPos());}
  |  LPAREN exp RPAREN  {$$ = $2;}
  |  LPAREN expseq RPAREN  {$$ = new absyn::SeqExp(scanner_.GetTokPos(), $2);}

  |  FOR ID ASSIGN exp TO exp DO exp  {$$ = new absyn::ForExp(scanner_.GetTokPos(), $2, $4, $6, $8);}

  |  IF exp THEN exp  {$$ = new absyn::IfExp(scanner_.GetTokPos(), $2, $4, nullptr);}
  |  IF exp THEN exp ELSE exp  {$$ = new absyn::IfExp(scanner_.GetTokPos(), $2, $4, $6);}
  |  IF LPAREN exp RPAREN THEN exp  {$$ = new absyn::IfExp(scanner_.GetTokPos(), $3, $6, nullptr);}
  |  IF LPAREN exp RPAREN THEN exp ELSE exp  {$$ = new absyn::IfExp(scanner_.GetTokPos(), $3, $6, $8);}

  |  WHILE exp DO exp  {$$ = new absyn::WhileExp(scanner_.GetTokPos(), $2, $4);}
  |  WHILE LPAREN exp RPAREN DO exp  {$$ = new absyn::WhileExp(scanner_.GetTokPos(), $3, $6);}

  |  ID LPAREN expseq RPAREN  {$$ = new absyn::CallExp(scanner_.GetTokPos(), $1, $3);}
  |  ID LPAREN RPAREN  {$$ = new absyn::CallExp(scanner_.GetTokPos(), $1, new absyn::ExpList());}

  |  ID LBRACE RBRACE  {$$ = new absyn::RecordExp(scanner_.GetTokPos(), $1, new absyn::EFieldList());}
  |  ID LBRACE rec RBRACE  {$$ = new absyn::RecordExp(scanner_.GetTokPos(), $1, $3);}

  |  MINUS exp %prec UMINUS  {$$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::MINUS_OP, new absyn::IntExp(scanner_.GetTokPos(), 0), $2);}
  |  exp PLUS exp  {$$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::PLUS_OP, $1, $3);}
  |  exp MINUS exp  {$$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::MINUS_OP, $1, $3);}
  |  exp DIVIDE exp  {$$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::DIVIDE_OP, $1, $3);}
  |  exp TIMES exp  {$$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::TIMES_OP, $1, $3);}
  |  exp EQ exp  {$$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::EQ_OP, $1, $3);}
  |  exp NEQ exp  {$$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::NEQ_OP, $1, $3);}
  |  exp LT exp  {$$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::LT_OP, $1, $3);}
  |  exp LE exp  {$$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::LE_OP, $1, $3);}
  |  exp GT exp  {$$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::GT_OP, $1, $3);}
  |  exp GE exp  {$$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::GE_OP, $1, $3);}
  |  exp AND exp  {$$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::AND_OP, $1, $3);}
  |  exp OR exp  {$$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::OR_OP, $1, $3);}
  ;
expseq:
     sequencing  {$$ = new absyn::SeqExp(scanner_.GetTokPos(), $1);}
  |  actuals  {$$ = new absyn::SeqExp(scanner_.GetTokPos(), $1);}
  ;

actuals:
     {$ = new absyn::VoidExp(scanner_.GetTokPos());}
  |  COMMA nonemptyactuals  {$$ = $1;}
  |  nonemptyactuals  {$$ = $1;}
  ;
nonemptyactuals:
     exp  {$$ = new absyn::ExpList($1);}
  |  exp COMMA nonemptyactuals  {$$ = $3; $$->Prepend($1);}
  ;
sequencing:
     {$ = new absyn::VoidExp(scanner_.GetTokPos());}
  |  SEMICOLON sequencing_exps  {$$ = $1;}
  |  sequencing_exps  {$$ = $1;}
  ;
sequencing_exps:
     exp  {$$ = new absyn::ExpList($1);}
  |  exp SEMICOLON sequencing_exps  {$$ = $3; $$->Prepend($1);}
  ;


decs:
     {$ = new absyn::VoidExp(scanner_.GetTokPos());}
  |  decs_nonempty_s  {$$ = $1;}
  ;
decs_nonempty:
     decs_nonempty_s  {$$ = new absyn::DecList($1);}
  |  decs_nonempty_s decs  {$$ = $2; $$->Prepend($1);}
  |  vardec  {$$ = new absyn::DecList($1);}
  |  vardec decs  {$$ = $2; $$->Prepend($1);}
  ;

decs_nonempty_s:
     tydec  {$$ = new absyn::TypeDec(scanner_.GetTokPos(), $1);}
  |  fundec  {$$ = new absyn::FunctionDec(scanner_.GetTokPos(), $1);}
  ;
vardec:
     VAR ID ASSIGN exp  {$$ = new absyn::VarDec(scanner_.GetTokPos(), $2, nullptr, $4);}
  |  VAR ID COLON ID ASSIGN exp  {$$ = new absyn::VarDec(scanner_.GetTokPos(), $2, $4, $6);}
  ;


rec:
     {$ = new absyn::VoidExp(scanner_.GetTokPos());}
  |  COMMA rec_nonempty  {$$ = $1;}
  |  rec_nonempty  {$$ = $1;}
  ;
rec_nonempty:
     rec_one  {$$ = new absyn::EFieldList($1);}
  |  rec_one COMMA rec  {$$ = $3; $$->Prepend($1);}
  ;

rec_one:
     ID EQ exp  {$$ = new absyn::EField($1, $3);};


tydec:
     TYPE tydec_one  {$$ = new absyn::NameAndTyList($2);}
  |  TYPE tydec_one tydec  {$$ = $3; $$->Prepend($2);}
  ;

tydec_one:
     ID EQ ty  {$$ = new absyn::NameAndTy($1, $3);};


tyfields:
     {$ = new absyn::VoidExp(scanner_.GetTokPos());}
  |  tyfields_nonempty  {$$ = $1;}
  ;
tyfields_nonempty:
     tyfield COMMA tyfields_nonempty  {$$ = $5; $$->Prepend(new absyn::Field(scanner_.GetTokPos(), $1, $3));};

tyfield:
     ID COLON ID  {$$ = new absyn::FieldList(new absyn::Field(scanner_.GetTokPos(), $1, $3));}


ty:
     ARRAY OF ID  {$$ = new absyn::ArrayTy(scanner_.GetTokPos(), $3);}
  |  ID  {$$ = new absyn::NameTy(scanner_.GetTokPos(), $1);}
  |  LBRACE tyfields RBRACE  {$$ = new absyn::RecordTy(scanner_.GetTokPos(), $2);}
  ;


fundec:
     FUNCTION fundec_one  {$$ = new absyn::FunDecList($2);}
  |  FUNCTION fundec_one fundec  {$$ = $3; $$->Prepend($2);}
  ;

fundec_one:
     ID LPAREN tyfields RPAREN COLON ID EQ exp  {$$ = new absyn::FunDec(scanner_.GetTokPos(), $1, $3, $6, $8);}
  |  ID LPAREN RPAREN COLON ID EQ exp  {$$ = new absyn::FunDec(scanner_.GetTokPos(), $1, new absyn::FieldList(), $5, $7);}
  |  ID LPAREN tyfields RPAREN EQ exp  {$$ = new absyn::FunDec(scanner_.GetTokPos(), $1, $3, nullptr, $6);}
  |  ID LPAREN RPAREN EQ exp  {$$ = new absyn::FunDec(scanner_.GetTokPos(), $1, new absyn::FieldList(), nullptr, $5);}
  |  ID LPAREN tyfields RPAREN COLON ID EQ LPAREN exp RPAREN  {$$ = new absyn::FunDec(scanner_.GetTokPos(), $1, $3, $6, $9);}
  |  ID LPAREN RPAREN COLON ID EQ LPAREN exp RPAREN  {$$ = new absyn::FunDec(scanner_.GetTokPos(), $1, new absyn::FieldList(), $5, $8);}
  |  ID LPAREN tyfields RPAREN EQ LPAREN exp RPAREN  {$$ = new absyn::FunDec(scanner_.GetTokPos(), $1, $3, nullptr, $7);}
  |  ID LPAREN RPAREN EQ LPAREN exp RPAREN  {$$ = new absyn::FunDec(scanner_.GetTokPos(), $1, new absyn::FieldList(), nullptr, $6);}
  ;
