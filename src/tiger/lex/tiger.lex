%filenames = "scanner"

 /*
  * Please don't modify the lines above.
  */

 /* You can add lex definitions here. */
 /* Put your lab2 code here */

%x COMMENT STR IGNORE

%%

 /*
  * Below is examples, which you can wipe out
  * and write regular expressions and actions of your own.
  *
  * All the tokens:
  *   Parser::ID
  *   Parser::STRING
  *   Parser::INT
  *   Parser::COMMA
  *   Parser::COLON
  *   Parser::SEMICOLON
  *   Parser::LPAREN
  *   Parser::RPAREN
  *   Parser::LBRACK
  *   Parser::RBRACK
  *   Parser::LBRACE
  *   Parser::RBRACE
  *   Parser::DOT
  *   Parser::PLUS
  *   Parser::MINUS
  *   Parser::TIMES
  *   Parser::DIVIDE
  *   Parser::EQ
  *   Parser::NEQ
  *   Parser::LT
  *   Parser::LE
  *   Parser::GT
  *   Parser::GE
  *   Parser::AND
  *   Parser::OR
  *   Parser::ASSIGN
  *   Parser::ARRAY
  *   Parser::IF
  *   Parser::THEN
  *   Parser::ELSE
  *   Parser::WHILE
  *   Parser::FOR
  *   Parser::TO
  *   Parser::DO
  *   Parser::LET
  *   Parser::IN
  *   Parser::END
  *   Parser::OF
  *   Parser::BREAK
  *   Parser::NIL
  *   Parser::FUNCTION
  *   Parser::VAR
  *   Parser::TYPE
  */

 /* reserved words */
"array" {adjust(); return Parser::ARRAY;}
 /* Put your lab2 code here */

\"            {
                adjust();
                flag = true;
                string_buf_ = "";
                begin(StartCondition__::STR);
              }
<STR>\"       {
                adjustStr();
                setMatched(string_buf_);
                begin(StartCondition__::INITIAL);
                if (flag)
                  return Parser::STRING;
              }
<STR>{
    \\            {adjustStr(); begin(StartCondition__::IGNORE);}
    \\n           {adjustStr(); string_buf_ += '\n';}
    \\t           {adjustStr(); string_buf_ += '\t';}
    \\^[A-Z]      {
                    adjustStr();
                    char ch = matched().at(2);
                    string_buf_ += (ch - 64);
                  }
    \\[0-9]{3}    {
                    adjustStr();
                    std::string str = matched();
                    string_buf_ += stoi(str.erase(0, 1));
                  }
    \\\"          {adjustStr(); string_buf_ += '\"';}
    \\\\          {adjustStr(); string_buf_ += '\\';}
    \n            {adjustStr(); errormsg_->Newline(); string_buf_ += '\n';}
    \r
}
<STR>.        {
                adjustStr();
                string_buf_ += matched().at(0);
              }
<STR><<EOF>>  {
                errormsg_->Error(errormsg_->tok_pos_, "EOF in string");
                return 0;
              }

<IGNORE>\\      {adjustStr(); begin(StartCondition__::STR);}
<IGNORE>\n      {adjustStr(); errormsg_->Newline();}
<IGNORE>\r
<IGNORE>[ \t]+  {adjustStr();}
<IGNORE><<EOF>> {
                  errormsg_->Error(errormsg_->tok_pos_, "EOF in ignore");
                  return 0;
                }




"/*"              {
                    adjust();
                    comment_level_ += 1;
                    begin(StartCondition__::COMMENT);
                  }
<COMMENT>"/*"     {adjust(); comment_level_ += 1;}
<COMMENT>"*/"     {
                    adjust();
                    comment_level_ -= 1;
                    if (comment_level_ == 1) {
                      begin(StartCondition__::INITIAL);
                    }
                  }
<COMMENT>\n       {adjust(); errormsg_->Newline();}
<COMMENT>\r
<COMMENT>.        adjust();
<COMMENT><<EOF>>  {
                    adjust();
                    errormsg_->Error(errormsg_->tok_pos_, "EOF in comment");
                    return 0;
                  }



"while"     {adjust(); return Parser::WHILE;}
"for"       {adjust(); return Parser::FOR;}
"break"     {adjust(); return Parser::BREAK;}
"let"       {adjust(); return Parser::LET;}
"in"        {adjust(); return Parser::IN;}
"end"       {adjust(); return Parser::END;}
"function"  {adjust(); return Parser::FUNCTION;}
"var"       {adjust(); return Parser::VAR;}
"type"      {adjust(); return Parser::TYPE;}
"if"        {adjust(); return Parser::IF;}
"then"      {adjust(); return Parser::THEN;}
"else"      {adjust(); return Parser::ELSE;}
"do"        {adjust(); return Parser::DO;}
"of"        {adjust(); return Parser::OF;}
"nil"       {adjust(); return Parser::NIL;}
"to"        {adjust(); return Parser::TO;}

"+" {adjust(); return Parser::PLUS;}
"-" {adjust(); return Parser::MINUS;}
"*" {adjust(); return Parser::TIMES;}
"/" {adjust(); return Parser::DIVIDE;}

"="   {adjust(); return Parser::EQ;}
"<>"  {adjust(); return Parser::NEQ;}
"<"   {adjust(); return Parser::LT;}
"<="  {adjust(); return Parser::LE;}
">"   {adjust(); return Parser::GT;}
">="  {adjust(); return Parser::GE;}

"&" {adjust(); return Parser::AND;}
"|" {adjust(); return Parser::OR;}

":=" {adjust(); return Parser::ASSIGN;}

[a-zA-Z]+[a-zA-Z0-9_]* {adjust(); return Parser::ID;}

[0-9]+ {adjust(); return Parser::INT;}


"." {adjust(); return Parser::DOT;}
"," {adjust(); return Parser::COMMA;}
":" {adjust(); return Parser::COLON;}
";" {adjust(); return Parser::SEMICOLON;}
"(" {adjust(); return Parser::LPAREN;}
")" {adjust(); return Parser::RPAREN;}
"[" {adjust(); return Parser::LBRACK;}
"]" {adjust(); return Parser::RBRACK;}
"{" {adjust(); return Parser::LBRACE;}
"}" {adjust(); return Parser::RBRACE;}


<<EOF>> return 0;


 /*
  * skip white space chars.
  * space, tabs and LF
  */
[ \t]+ {adjust();}
\n {adjust(); errormsg_->Newline();}

 /* illegal input */
. {adjust(); errormsg_->Error(errormsg_->tok_pos_, "illegal token");}
