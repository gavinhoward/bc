#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <bc/lex.h>
#include <bc/parse.h>
#include <bc/instructions.h>

// This is an array that corresponds to token types. An entry is
// true if the token is valid in an expression, false otherwise.
static const bool bc_token_exprs[] = {

  true,
  true,

  true,

  true,
  true,
  true,

  true,
  true,

  true,
  true,
  true,
  true,
  true,
  true,
  true,

  true,
  true,
  true,
  true,
  true,
  true,

  true,

  true,
  true,

  true,

  false,

  false,

  true,
  true,

  false,
  false,

  false,
  false,

  false,
  false,

  false,
  true,
  true,

  false,
  false,
  false,
  false,
  false,
  false,
  false,
  true,
  false,
  true,
  true,
  true,
  true,
  false,
  false,
  true,
  false,
  true,
  true,
  false,

  false,

  false,
};

// This is an array of data for operators that correspond to token types.
// The last corresponds to BC_PARSE_OP_NEGATE_IDX since it doesn't have
// its own token type (it is the same token at the binary minus operator).
static const BcOp bc_ops[] = {

  { 0, false },
  { 0, false },

  { 2, false },

  { 3, true },
  { 3, true },
  { 3, true },

  { 4, true },
  { 4, true },

  { 5, false },
  { 5, false },
  { 5, false },
  { 5, false },
  { 5, false },
  { 5, false },
  { 5, false },

  { 6, true },
  { 6, true },
  { 6, true },
  { 6, true },
  { 6, true },
  { 6, true },

  { 7, false },

  { 8, true },
  { 8, true },

  { 1, false }

};

static BcStatus bc_parse_func(BcParse* parse, BcProgram* program);
static BcStatus bc_parse_funcStart(BcParse* parse);
static BcStatus bc_parse_auto(BcParse* parse);
static BcStatus bc_parse_semicolonList(BcParse* parse, BcVec *code);
static BcStatus bc_parse_semicolonListEnd(BcParse* parse, BcVec *code);
static BcStatus bc_parse_stmt(BcParse* parse, BcVec *code);
static BcStatus bc_parse_operator(BcParse* parse, BcVec* exs, BcVec* ops,
                                  BcLexTokenType t, uint32_t* num_exprs,
                                  bool next);
static BcStatus bc_parse_rightParen(BcParse* parse, BcVec* exs,
                                    BcVec* ops, uint32_t* nexs);
static BcStatus bc_parse_expr_name(BcParse* parse, BcVec* exprs,
                                   BcExprType* type, bool posix_rel);
static BcStatus bc_parse_call(BcParse* parse, BcVec* code, bool posix_rel);
static BcStatus bc_parse_params(BcParse* parse, BcVec* code, bool posix_rel);
static BcStatus bc_parse_read(BcParse* parse, BcVec* exprs);
static BcStatus bc_parse_read(BcParse* parse, BcVec* exprs);
static BcStatus bc_parse_read(BcParse* parse, BcVec* exprs);
static BcStatus bc_parse_builtin(BcParse* parse, BcVec* exs,
                                 BcExprType type, bool posix_rel);
static BcStatus bc_parse_scale(BcParse* parse, BcVec* exs,
                               BcExprType* type, bool posix_rel);
static BcStatus bc_parse_incdec(BcParse* parse, BcVec* exs, BcExprType* prev);
static BcStatus bc_parse_minus(BcParse* parse, BcVec* exs, BcVec* ops,
                               BcExprType* prev, bool rparen, uint32_t* nexprs);
static BcStatus bc_parse_string(BcParse* parse, BcVec *code);
static BcStatus bc_parse_return(BcParse* parse, BcStmtList* list);
static BcStatus bc_parse_print(BcParse* parse, BcStmtList* list);
static BcStatus bc_parse_if(BcParse* parse, BcStmtList* list);
static BcStatus bc_parse_while(BcParse* parse, BcStmtList* list);
static BcStatus bc_parse_for(BcParse* parse, BcStmtList* list);
static BcStatus bc_parse_startBody(BcParse* parse, BcVec **code,
                                   uint8_t flags);
static BcStatus bc_parse_loopExit(BcParse* parse, BcStmtList* list,
                                  BcLexTokenType type);
static BcStatus bc_parse_rightBrace(BcParse* parse);
static BcStatus bc_parse_pushNum(BcVec* code, const char* num);
static BcStatus bc_parse_pushIndex(BcVec* code, size_t idx);

BcStatus bc_parse_init(BcParse* parse, BcProgram* program, BcVec *code) {

  if (parse == NULL || code == NULL) {
    return BC_STATUS_INVALID_PARAM;
  }

  BcStatus status = bc_vec_init(&parse->flag_stack, sizeof(uint8_t), NULL);

  if (status != BC_STATUS_SUCCESS) {
    return status;
  }

  status = bc_vec_init(&parse->ctx_stack, sizeof(BcVec*), NULL);

  if (status != BC_STATUS_SUCCESS) {
    goto ctx_err;
  }

  uint8_t flags = 0;

  status = bc_vec_push(&parse->flag_stack, &flags);
  if (status != BC_STATUS_SUCCESS) {
    goto push_err;
  }

  status = bc_vec_push(&parse->ctx_stack, &code);

  if (status) {
    goto push_err;
  }

  status = bc_vec_init(&parse->ops, sizeof(BcLexTokenType), NULL);

  if (status) {
    goto push_err;
  }

  parse->program = program;
  parse->func = NULL;
  parse->num_braces = 0;
  parse->auto_part = false;

  return status;

push_err:

  bc_vec_free(&parse->ctx_stack);

ctx_err:

  bc_vec_free(&parse->flag_stack);

  return status;
}

BcStatus bc_parse_file(BcParse* parse, const char* file) {

  if (parse == NULL || file == NULL) {
    return BC_STATUS_INVALID_PARAM;
  }

  return bc_lex_init(&parse->lex, file);
}

BcStatus bc_parse_text(BcParse* parse, const char* text) {

  BcStatus status;

  if (parse == NULL || text == NULL) {
    return BC_STATUS_INVALID_PARAM;
  }

  status = bc_lex_text(&parse->lex, text);

  if (status) {
    return status;
  }

  return bc_lex_next(&parse->lex, &parse->token);
}

BcStatus bc_parse_parse(BcParse* parse, BcProgram* program) {

  BcVec** ptr;
  BcStatus status;

  if (parse == NULL) {
    return BC_STATUS_INVALID_PARAM;
  }

  status = BC_STATUS_SUCCESS;

  switch (parse->token.type) {

    case BC_LEX_NEWLINE:
    {
      status = bc_lex_next(&parse->lex, &parse->token);
      break;
    }

    case BC_LEX_KEY_DEFINE:
    {
      if (!BC_PARSE_CAN_EXEC(parse)) {
        return BC_STATUS_PARSE_INVALID_TOKEN;
      }

      status = bc_parse_func(parse, program);

      break;
    }

    case BC_LEX_EOF:
    {
      status = BC_STATUS_PARSE_EOF;
      break;
    }

    default:
    {
      ptr = bc_vec_top(&parse->ctx_stack);
      status = bc_parse_stmt(parse, *ptr);
      break;
    }
  }

  return status;
}

void bc_parse_free(BcParse* parse) {

  if (!parse) {
    return;
  }

  bc_vec_free(&parse->flag_stack);
  bc_vec_free(&parse->ctx_stack);
  bc_vec_free(&parse->ops);

  switch (parse->token.type) {

    case BC_LEX_STRING:
    case BC_LEX_NAME:
    case BC_LEX_NUMBER:
    {
      if (parse->token.string) {
        free(parse->token.string);
      }

      break;
    }

    default:
    {
      // We don't have have to free anything.
      break;
    }
  }
}

static BcStatus bc_parse_func(BcParse* parse, BcProgram* program) {

  BcLexTokenType type;
  BcStatus status;
  BcFunc func;
  bool comma;
  uint8_t flags;
  char* name;
  bool var;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) {
    return status;
  }

  if (parse->token.type != BC_LEX_NAME) {
    return BC_STATUS_PARSE_INVALID_FUNC;
  }

  status = bc_func_init(&func, parse->token.string);

  if (status) {
    return status;
  }

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) {
    goto lex_err;
  }

  if (parse->token.type != BC_LEX_LEFT_PAREN) {
    status = BC_STATUS_PARSE_INVALID_FUNC;
    goto lex_err;
  }

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) {
    goto lex_err;
  }

  comma = false;

  type = parse->token.type;

  while (!status && type != BC_LEX_RIGHT_PAREN) {

    if (type != BC_LEX_NAME) {
      status = BC_STATUS_PARSE_INVALID_FUNC;
      goto lex_err;
    }

    name = parse->token.string;

    status = bc_lex_next(&parse->lex, &parse->token);

    if (status) {
      goto lex_err;
    }

    if (parse->token.type == BC_LEX_LEFT_BRACKET) {

      status = bc_lex_next(&parse->lex, &parse->token);

      if (status) {
        goto lex_err;
      }

      if (parse->token.type != BC_LEX_RIGHT_BRACKET) {
        return BC_STATUS_PARSE_INVALID_FUNC;
      }

      status = bc_lex_next(&parse->lex, &parse->token);

      if (status) {
        goto lex_err;
      }

      var = false;
    }
    else {
      var = true;
    }

    if (parse->token.type == BC_LEX_COMMA) {

      comma = true;
      status = bc_lex_next(&parse->lex, &parse->token);

      if (status) {
        goto lex_err;
      }
    }
    else {
      comma = false;
    }

    status = bc_func_insertParam(&func, name, var);

    if (status) {
      goto lex_err;
    }

    type = parse->token.type;
  }

  if (comma) {
    return BC_STATUS_PARSE_INVALID_FUNC;
  }

  flags = BC_PARSE_FLAG_FUNC | BC_PARSE_FLAG_FUNC_INNER | BC_PARSE_FLAG_HEADER;
  status = bc_vec_push(&parse->flag_stack, &flags);

  if (status) {
    goto lex_err;
  }

  status = bc_vec_push(&parse->ctx_stack, &func.first);

  if (status) {
    goto lex_err;
  }

  status = bc_vec_push(&program->funcs, &func);

  if (status) {
    goto lex_err;
  }

  parse->func = bc_vec_item(&program->funcs, program->funcs.len - 1);

  if (!parse->func) {
    return BC_STATUS_PARSE_BUG;
  }

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) {
    return status;
  }

  if (parse->token.type != BC_LEX_LEFT_BRACE) {
    return bc_posix_error(BC_STATUS_POSIX_FUNC_HEADER_LEFT_BRACE,
                          parse->lex.file, parse->lex.line, NULL);
  }

  return status;

lex_err:

  bc_func_free(&func);

  return status;
}

static BcStatus bc_parse_funcStart(BcParse* parse) {

  BcStatus status;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) {
    return status;
  }

  parse->auto_part = true;

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_parse_auto(BcParse* parse) {

  BcLexTokenType type;
  BcStatus status;
  bool comma;
  char* name;
  bool var;
  bool one;

  if (!parse->auto_part) {
    return BC_STATUS_PARSE_INVALID_TOKEN;
  }

  parse->auto_part = false;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) {
    return status;
  }

  comma = false;
  one = false;

  type = parse->token.type;

  while (!status && type == BC_LEX_NAME) {

    name = parse->token.string;

    status = bc_lex_next(&parse->lex, &parse->token);

    if (status) {
      break;
    }

    one = true;

    if (parse->token.type == BC_LEX_LEFT_BRACKET) {

      status = bc_lex_next(&parse->lex, &parse->token);

      if (status) {
        break;
      }

      if (parse->token.type != BC_LEX_RIGHT_BRACKET) {
        return BC_STATUS_PARSE_INVALID_FUNC;
      }

      status = bc_lex_next(&parse->lex, &parse->token);

      if (status) {
        break;
      }

      var = false;
    }
    else {
      var = true;
    }

    if (parse->token.type == BC_LEX_COMMA) {
      comma = true;
      status = bc_lex_next(&parse->lex, &parse->token);
    }
    else {
      comma = false;
    }

    status = bc_func_insertAuto(parse->func, name, var);

    if (status) {
      free(name);
      return status;
    }

    type = parse->token.type;
  }

  if (status) {
    return status;
  }

  if (comma) {
    return BC_STATUS_PARSE_INVALID_FUNC;
  }

  if (!one) {
    return BC_STATUS_PARSE_NO_AUTO;
  }

  if (type != BC_LEX_NEWLINE && type != BC_LEX_SEMICOLON) {
    return BC_STATUS_PARSE_INVALID_TOKEN;
  }

  return bc_lex_next(&parse->lex, &parse->token);
}

static BcStatus bc_parse_semicolonList(BcParse* parse, BcVec* code) {

  BcStatus status = BC_STATUS_SUCCESS;

  switch (parse->token.type) {

    case BC_LEX_OP_INC:
    case BC_LEX_OP_DEC:
    case BC_LEX_OP_MINUS:
    case BC_LEX_OP_BOOL_NOT:
    case BC_LEX_LEFT_PAREN:
    case BC_LEX_STRING:
    case BC_LEX_NAME:
    case BC_LEX_NUMBER:
    case BC_LEX_KEY_BREAK:
    case BC_LEX_KEY_CONTINUE:
    case BC_LEX_KEY_FOR:
    case BC_LEX_KEY_HALT:
    case BC_LEX_KEY_IBASE:
    case BC_LEX_KEY_IF:
    case BC_LEX_KEY_LAST:
    case BC_LEX_KEY_LENGTH:
    case BC_LEX_KEY_LIMITS:
    case BC_LEX_KEY_OBASE:
    case BC_LEX_KEY_PRINT:
    case BC_LEX_KEY_QUIT:
    case BC_LEX_KEY_READ:
    case BC_LEX_KEY_RETURN:
    case BC_LEX_KEY_SCALE:
    case BC_LEX_KEY_SQRT:
    case BC_LEX_KEY_WHILE:
    {
      status = bc_parse_stmt(parse, code);
      break;
    }

    case BC_LEX_NEWLINE:
    {
      status = bc_lex_next(&parse->lex, &parse->token);
      break;
    }

    case BC_LEX_SEMICOLON:
    {
      status = bc_parse_semicolonListEnd(parse, code);
      break;
    }

    case BC_LEX_EOF:
    {
      if (parse->ctx_stack.len > 0) {
        status = BC_STATUS_PARSE_EOF;
      }

      break;
    }

    default:
    {
      status = BC_STATUS_PARSE_INVALID_TOKEN;
      break;
    }
  }

  return status;
}

static BcStatus bc_parse_semicolonListEnd(BcParse* parse, BcVec* code) {

  BcStatus status;

  if (parse->token.type == BC_LEX_SEMICOLON) {

    status = bc_lex_next(&parse->lex, &parse->token);

    if (status) {
      return status;
    }

    status = bc_parse_semicolonList(parse, code);
  }
  else if (parse->token.type == BC_LEX_NEWLINE) {
    status = bc_lex_next(&parse->lex, &parse->token);
  }
  else {
    status = BC_STATUS_PARSE_INVALID_TOKEN;
  }

  return status;
}

static BcStatus bc_parse_stmt(BcParse* parse, BcVec* code) {

  BcStatus status;
  BcStmt stmt;
  uint8_t* flag_ptr;

  status = BC_STATUS_SUCCESS;

  switch (parse->token.type) {

    case BC_LEX_OP_INC:
    case BC_LEX_OP_DEC:
    case BC_LEX_OP_MINUS:
    case BC_LEX_OP_BOOL_NOT:
    case BC_LEX_LEFT_PAREN:
    case BC_LEX_NAME:
    case BC_LEX_NUMBER:
    case BC_LEX_KEY_IBASE:
    case BC_LEX_KEY_LAST:
    case BC_LEX_KEY_LENGTH:
    case BC_LEX_KEY_OBASE:
    case BC_LEX_KEY_READ:
    case BC_LEX_KEY_SCALE:
    case BC_LEX_KEY_SQRT:
    {
      parse->auto_part = false;

      //status = bc_stmt_init(&stmt, BC_STMT_EXPR);

      if (status) {
        break;
      }

      //status = bc_parse_expr(parse, stmt.data.exprs, false);

      if (status) {
        bc_stmt_free(&stmt);
        break;
      }

      status = bc_list_insert(code, &stmt);

      if (status) {
        bc_stmt_free(&stmt);
        break;
      }

      status = bc_parse_semicolonListEnd(parse, code);

      break;
    }

    case BC_LEX_NEWLINE:
    {
      status = bc_lex_next(&parse->lex, &parse->token);
      break;
    }

    case BC_LEX_LEFT_BRACE:
    {
      parse->auto_part = false;

      ++parse->num_braces;

      if (BC_PARSE_HEADER(parse)) {

        flag_ptr = bc_vec_top(&parse->flag_stack);
        *flag_ptr = *flag_ptr & ~(BC_PARSE_FLAG_HEADER);

        if (BC_PARSE_FUNC_INNER(parse)) {
          status = bc_parse_funcStart(parse);
        }
        else if (BC_PARSE_FOR_LOOP(parse) ||
                 BC_PARSE_WHILE_LOOP(parse) ||
                 BC_PARSE_IF(parse))
        {
          status = bc_parse_if(parse, code);
        }
        else {
          status = BC_STATUS_PARSE_BUG;
        }
      }
      else {

        //status = bc_stmt_init(&stmt, BC_STMT_LIST);

        if (status) {
          break;
        }

        //status = bc_parse_startBody(parse, &stmt.data.list, 0);

        if (status) {
          bc_stmt_free(&stmt);
        }
      }

      break;
    }

    case BC_LEX_RIGHT_BRACE:
    {
      parse->auto_part = false;

      status = bc_parse_rightBrace(parse);

      break;
    }

    case BC_LEX_STRING:
    {
      parse->auto_part = false;

      status = bc_parse_string(parse, code);

      break;
    }

    case BC_LEX_KEY_AUTO:
    {
      status = bc_parse_auto(parse);
      break;
    }

    case BC_LEX_KEY_BREAK:
    case BC_LEX_KEY_CONTINUE:
    {
      parse->auto_part = false;

      status = bc_parse_loopExit(parse, code, parse->token.type);

      break;
    }

    case BC_LEX_KEY_FOR:
    {
      parse->auto_part = false;

      status = bc_parse_for(parse, code);

      break;
    }

    case BC_LEX_KEY_HALT:
    {
      parse->auto_part = false;

      //status = bc_stmt_init(&stmt, BC_STMT_HALT);

      if (status) {
        break;
      }

      status = bc_list_insert(code, &stmt);

      if (status) {
        bc_stmt_free(&stmt);
        break;
      }

      status = bc_lex_next(&parse->lex, &parse->token);

      if (status) {
        break;
      }

      status = bc_parse_semicolonListEnd(parse, code);

      break;
    }

    case BC_LEX_KEY_IF:
    {
      parse->auto_part = false;

      status = bc_parse_if(parse, code);

      break;
    }

    case BC_LEX_KEY_LIMITS:
    {
      status = bc_lex_next(&parse->lex, &parse->token);

      if (status && status != BC_STATUS_LEX_EOF) {
        break;
      }

      status = bc_parse_semicolonListEnd(parse, code);

      if (status && status != BC_STATUS_LEX_EOF) {
        break;
      }

      status = BC_STATUS_PARSE_LIMITS;

      break;
    }

    case BC_LEX_KEY_PRINT:
    {
      parse->auto_part = false;

      status = bc_parse_print(parse, code);

      break;
    }

    case BC_LEX_KEY_QUIT:
    {
      // Quit is a compile-time command,
      // so we send an exit command. We
      // don't exit directly, so the vm
      // can clean up.
      status = BC_STATUS_PARSE_QUIT;
      break;
    }

    case BC_LEX_KEY_RETURN:
    {
      parse->auto_part = false;

      status = bc_parse_return(parse, code);

      if (status) {
        break;
      }

      status = bc_parse_semicolonListEnd(parse, code);

      break;
    }

    case BC_LEX_KEY_WHILE:
    {
      parse->auto_part = false;

      status = bc_parse_while(parse, code);

      break;
    }

    default:
    {
      parse->auto_part = false;

      status = BC_STATUS_PARSE_INVALID_TOKEN;

      break;
    }
  }

  return status;
}

BcStatus bc_parse_expr(BcParse* parse, BcVec* code, bool posix_rel) {

  BcStatus status;
  uint32_t nexprs;
  uint32_t num_parens;
  uint32_t ops_start_len;
  bool paren_first;
  bool paren_expr;
  bool rparen;
  bool done;
  bool get_token;
  uint32_t num_rel_ops;
  BcExprType prev;
  BcLexTokenType type;
  BcLexTokenType* ptr;
  BcLexTokenType top;

  ops_start_len = parse->ops.len;

  prev = BC_EXPR_PRINT;

  paren_first = parse->token.type == BC_LEX_LEFT_PAREN;

  if (status) {
    return status;
  }

  nexprs = 0;
  num_parens = 0;
  paren_expr = false;
  rparen = false;
  done = false;
  get_token = false;
  num_rel_ops = 0;

  type = parse->token.type;

  while (!status && !done && bc_token_exprs[type]) {

    switch (type) {

      case BC_LEX_OP_INC:
      case BC_LEX_OP_DEC:
      {
        status = bc_parse_incdec(parse, code, &prev);
        rparen = false;
        get_token = false;
        break;
      }

      case BC_LEX_OP_MINUS:
      {
        status = bc_parse_minus(parse, code, &parse->ops, &prev,
                                rparen, &nexprs);
        rparen = false;
        get_token = false;
        break;
      }

      case BC_LEX_OP_ASSIGN_POWER:
      case BC_LEX_OP_ASSIGN_MULTIPLY:
      case BC_LEX_OP_ASSIGN_DIVIDE:
      case BC_LEX_OP_ASSIGN_MODULUS:
      case BC_LEX_OP_ASSIGN_PLUS:
      case BC_LEX_OP_ASSIGN_MINUS:
      case BC_LEX_OP_ASSIGN:
        if (prev != BC_EXPR_VAR && prev != BC_EXPR_ARRAY_ELEM &&
            prev != BC_EXPR_SCALE && prev != BC_EXPR_IBASE &&
            prev != BC_EXPR_OBASE && prev != BC_EXPR_LAST)
        {
          status = BC_STATUS_PARSE_INVALID_ASSIGN;
          break;
        }
        // Fallthrough.
      case BC_LEX_OP_POWER:
      case BC_LEX_OP_MULTIPLY:
      case BC_LEX_OP_DIVIDE:
      case BC_LEX_OP_MODULUS:
      case BC_LEX_OP_PLUS:
      case BC_LEX_OP_REL_EQUAL:
      case BC_LEX_OP_REL_LESS_EQ:
      case BC_LEX_OP_REL_GREATER_EQ:
      case BC_LEX_OP_REL_NOT_EQ:
      case BC_LEX_OP_REL_LESS:
      case BC_LEX_OP_REL_GREATER:
      case BC_LEX_OP_BOOL_NOT:
      case BC_LEX_OP_BOOL_OR:
      case BC_LEX_OP_BOOL_AND:
      {
        if (type >= BC_LEX_OP_REL_EQUAL && type <= BC_LEX_OP_REL_GREATER) {
          num_rel_ops += 1;
        }

        prev = BC_PARSE_TOKEN_TO_EXPR(type);
        status = bc_parse_operator(parse, code, &parse->ops, type, &nexprs, true);
        rparen = false;
        get_token = false;

        break;
      }

      case BC_LEX_LEFT_PAREN:
      {
        ++num_parens;
        paren_expr = false;
        rparen = false;
        get_token = true;
        status = bc_vec_push(&parse->ops, &type);
        break;
      }

      case BC_LEX_RIGHT_PAREN:
      {
        if (num_parens == 0) {
          status = BC_STATUS_SUCCESS;
          done = true;
          get_token = false;
          break;
        }
        else if (!paren_expr) {
          return BC_STATUS_PARSE_INVALID_EXPR;
        }

        --num_parens;
        paren_expr = true;
        rparen = true;
        get_token = false;

        status = bc_parse_rightParen(parse, code, &parse->ops, &nexprs);

        break;
      }

      case BC_LEX_NAME:
      {
        paren_expr = true;
        rparen = false;
        get_token = false;
        status = bc_parse_expr_name(parse, code, &prev, posix_rel);
        ++nexprs;
        break;
      }

      case BC_LEX_NUMBER:
      {
        expr.type = BC_EXPR_NUMBER;
        expr.string = parse->token.string;
        status = bc_vec_push(code, &expr);

        paren_expr = true;
        rparen = false;
        get_token = true;
        ++nexprs;
        prev = BC_EXPR_NUMBER;

        break;
      }

      case BC_LEX_KEY_IBASE:
      {
        expr.type = BC_EXPR_IBASE;
        status = bc_vec_push(code, &expr);

        paren_expr = true;
        rparen = false;
        get_token = true;
        ++nexprs;
        prev = BC_EXPR_IBASE;

        break;
      }

      case BC_LEX_KEY_LENGTH:
      {
        status = bc_parse_builtin(parse, code, BC_EXPR_LENGTH, posix_rel);
        paren_expr = true;
        rparen = false;
        get_token = true;
        ++nexprs;
        prev = BC_EXPR_LENGTH;
        break;
      }

      case BC_LEX_KEY_OBASE:
      {
        expr.type = BC_EXPR_OBASE;
        status = bc_vec_push(code, &expr);

        paren_expr = true;
        rparen = false;
        get_token = true;
        ++nexprs;
        prev = BC_EXPR_OBASE;

        break;
      }

      case BC_LEX_KEY_READ:
      {
        status = bc_parse_read(parse, code);
        paren_expr = true;
        rparen = false;
        get_token = false;
        ++nexprs;
        prev = BC_EXPR_READ;
        break;
      }

      case BC_LEX_KEY_SCALE:
      {
        status = bc_parse_scale(parse, code, &prev, posix_rel);
        paren_expr = true;
        rparen = false;
        get_token = false;
        ++nexprs;
        prev = BC_EXPR_SCALE;
        break;
      }

      case BC_LEX_KEY_SQRT:
      {
        status = bc_parse_builtin(parse, code, BC_EXPR_SQRT, posix_rel);
        paren_expr = true;
        rparen = false;
        get_token = false;
        ++nexprs;
        prev = BC_EXPR_SQRT;
        break;
      }

      default:
      {
        status = BC_STATUS_PARSE_INVALID_TOKEN;
        break;
      }
    }

    if (get_token) {
      status = bc_lex_next(&parse->lex, &parse->token);
    }

    type = parse->token.type;
  }

  if (status) {
    goto expr_err;
  }

  while (!status && parse->ops.len > ops_start_len) {

    ptr = bc_vec_top(&parse->ops);
    top = *ptr;

    if (top == BC_LEX_LEFT_PAREN || top == BC_LEX_RIGHT_PAREN) {
      status = BC_STATUS_PARSE_INVALID_EXPR;
      goto expr_err;
    }

    expr.type = BC_PARSE_TOKEN_TO_EXPR(top);

    status = bc_vec_push(code, &expr);

    if (status) {
      goto expr_err;
    }

    nexprs -= top != BC_LEX_OP_BOOL_NOT && top != BC_LEX_OP_NEGATE ? 1 : 0;

    status = bc_vec_pop(&parse->ops);
  }

  if (nexprs != 1) {
    status = BC_STATUS_PARSE_INVALID_EXPR;
    goto expr_err;
  }

  if (!posix_rel && num_rel_ops &&
      (status = bc_posix_error(BC_STATUS_POSIX_REL_OUTSIDE,
                               parse->lex.file, parse->lex.line, NULL)))
  {
    return status;
  }
  else if (num_rel_ops > 1 &&
           (status = bc_posix_error(BC_STATUS_POSIX_REL_OUTSIDE,
                                    parse->lex.file, parse->lex.line, NULL)))
  {
    return status;
  }

  expr.type = ((BcExpr*) bc_vec_top(code))->type;

  if (expr.type < BC_EXPR_ASSIGN_POWER ||
      expr.type > BC_EXPR_ASSIGN ||
      paren_first)
  {
    expr.type = BC_EXPR_PRINT;
    status = bc_vec_push(code, &expr);

    if (status) {
      goto expr_err;
    }
  }

  return status;

expr_err:

  bc_vec_free(code);

  return status;
}

static BcStatus bc_parse_operator(BcParse* parse, BcVec* exs, BcVec* ops,
                                  BcLexTokenType t, uint32_t* num_exprs,
                                  bool next)
{
  BcExpr expr;
  BcStatus status;
  BcLexTokenType top;
  BcLexTokenType* ptr;
  uint8_t lp;
  uint8_t rp;
  bool rleft;

  rp = bc_ops[t].prec;
  rleft = bc_ops[t].left;

  if (ops->len != 0) {

    ptr = bc_vec_top(ops);
    top = *ptr;

    if (top != BC_LEX_LEFT_PAREN) {

      lp = bc_ops[top].prec;

      while (lp < rp || (lp == rp && rleft)) {

        expr.type = BC_PARSE_TOKEN_TO_EXPR(top);

        status = bc_vec_push(exs, &expr);

        if (status) {
          return status;
        }

        status = bc_vec_pop(ops);

        if (status) {
          return status;
        }

        *num_exprs -= top != BC_LEX_OP_BOOL_NOT &&
                      top != BC_LEX_OP_NEGATE ? 1 : 0;

        if (ops->len == 0) {
          break;
        }

        ptr = bc_vec_top(ops);
        top = *ptr;

        if (top == BC_LEX_LEFT_PAREN) {
          break;
        }

        lp = bc_ops[top].prec;
      }
    }
  }

  status = bc_vec_push(ops, &t);

  if (status) {
    return status;
  }

  if (next) {
    status = bc_lex_next(&parse->lex, &parse->token);
  }

  return status;
}

static BcStatus bc_parse_rightParen(BcParse* parse, BcVec* exs,
                                    BcVec* ops, uint32_t* nexs)
{
  BcStatus status;
  BcExpr expr;
  BcLexTokenType top;
  BcLexTokenType* ptr;

  if (ops->len == 0) {
    return BC_STATUS_PARSE_INVALID_EXPR;
  }

  ptr = bc_vec_top(ops);
  top = *ptr;

  while (top != BC_LEX_LEFT_PAREN) {

    expr.type = BC_PARSE_TOKEN_TO_EXPR(top);

    status = bc_vec_push(exs, &expr);

    if (status) {
      return status;
    }

    status = bc_vec_pop(ops);

    if (status) {
      return status;
    }

    *nexs -= top != BC_LEX_OP_BOOL_NOT &&
             top != BC_LEX_OP_NEGATE ? 1 : 0;

    if (ops->len == 0) {
      return BC_STATUS_PARSE_INVALID_EXPR;
    }

    ptr = bc_vec_top(ops);
    top = *ptr;
  }

  status = bc_vec_pop(ops);

  if (status) {
    return status;
  }

  return bc_lex_next(&parse->lex, &parse->token);
}

static BcStatus bc_parse_expr_name(BcParse* parse, BcVec* exprs,
                                   BcExprType* type, bool posix_rel)
{
  BcStatus status;
  BcExpr expr;
  char* name;

  name = parse->token.string;
  expr.exprs = NULL;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) {
    return status;
  }

  if (parse->token.type == BC_LEX_LEFT_BRACKET) {

    expr.type = BC_EXPR_ARRAY_ELEM;
    *type = BC_EXPR_ARRAY_ELEM;
    expr.string = name;

    expr.exprs = malloc(sizeof(BcVec));

    if (!expr.exprs) {
      return BC_STATUS_MALLOC_FAIL;
    }

    status = bc_lex_next(&parse->lex, &parse->token);

    if (status) {
      goto name_err;
    }

    status = bc_parse_expr(parse, expr.exprs, posix_rel);

    if (status) {
      goto name_err;
    }

    if (parse->token.type != BC_LEX_RIGHT_BRACKET) {
      status = BC_STATUS_PARSE_INVALID_TOKEN;
      goto name_err;
    }
  }
  else if (parse->token.type == BC_LEX_LEFT_PAREN) {

    *type = BC_EXPR_FUNC_CALL;

    expr.call = bc_call_create();

    if (!expr.call) {
      return BC_STATUS_MALLOC_FAIL;
    }

    expr.call->name = name;

    status = bc_parse_call(parse, &expr, posix_rel);

    if (status) {
      free(expr.call);
    }
  }
  else {
    expr.type = BC_EXPR_VAR;
    *type = BC_EXPR_VAR;
    expr.string = parse->token.string;
  }

  status = bc_vec_push(exprs, &expr);

  if (status && expr.exprs) {
    goto name_err;
  }

  return status;

name_err:

  free(expr.exprs);
  expr.exprs = NULL;

  return status;
}

static BcStatus bc_parse_call(BcParse* parse, BcVec* code, bool posix_rel) {

  BcStatus status;

  status = bc_parse_params(parse, code, posix_rel);

  if (status) {
    return status;
  }

  if (parse->token.type != BC_LEX_RIGHT_PAREN) {
    return BC_STATUS_PARSE_INVALID_TOKEN;
  }

  return bc_lex_next(&parse->lex, &parse->token);
}

static BcStatus bc_parse_params(BcParse* parse, BcVec* code, bool posix_rel) {

  BcStatus status;
  bool comma;
  size_t nparams;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) {
    return status;
  }

  if (parse->token.type == BC_LEX_RIGHT_PAREN) {

    status = bc_vec_pushByte(code, BC_INST_CALL);

    if (status) {
      return status;
    }

    return bc_vec_pushByte(code, 0);
  }

  comma = false;
  nparams = 0;

  do {

    ++nparams;

    status = bc_parse_expr(parse, code, posix_rel);

    if (status) {
      return status;
    }

    if (parse->token.type == BC_LEX_COMMA) {

      comma = true;
      status = bc_lex_next(&parse->lex, &parse->token);

      if (status) {
        return status;
      }
    }
    else {
      comma = false;
    }

  } while (!status && parse->token.type != BC_LEX_RIGHT_PAREN);

  if (status) {
    return status;
  }

  if (comma) {
    return BC_STATUS_PARSE_INVALID_TOKEN;
  }

  status = bc_vec_pushByte(code, BC_INST_CALL);

  if (status) {
    return status;
  }

  return bc_parse_pushIndex(code, nparams);
}

static BcStatus bc_parse_read(BcParse* parse, BcVec* exprs) {

  BcStatus status;
  BcExpr expr;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) {
    return status;
  }

  if (parse->token.type != BC_LEX_LEFT_PAREN) {
    return BC_STATUS_PARSE_INVALID_TOKEN;
  }

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) {
    return status;
  }

  if (parse->token.type != BC_LEX_RIGHT_PAREN) {
    return BC_STATUS_PARSE_INVALID_TOKEN;
  }

  expr.type = BC_EXPR_READ;

  status = bc_vec_push(exprs, &expr);

  if (status) {
    return status;
  }

  return bc_lex_next(&parse->lex, &parse->token);
}

static BcStatus bc_parse_builtin(BcParse* parse, BcVec* exs,
                                 BcExprType type, bool posix_rel)
{
  BcStatus status;
  BcExpr expr;

  expr.type = type;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) {
    return status;
  }

  if (parse->token.type != BC_LEX_LEFT_PAREN) {
    return BC_STATUS_PARSE_INVALID_TOKEN;
  }

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) {
    return status;
  }

  expr.exprs = malloc(sizeof(BcVec));

  if (!expr.exprs) {
    return BC_STATUS_MALLOC_FAIL;
  }

  status = bc_parse_expr(parse, expr.exprs, posix_rel);

  if (status) {
    goto init_err;
  }

  if (parse->token.type != BC_LEX_RIGHT_PAREN) {
    status = BC_STATUS_PARSE_INVALID_TOKEN;
    goto init_err;
  }

  status = bc_vec_push(exs, &expr);

  if (status) {
    goto init_err;
  }

  return bc_lex_next(&parse->lex, &parse->token);

init_err:

  free(expr.exprs);

  return status;
}

static BcStatus bc_parse_scale(BcParse* parse, BcVec* exs,
                               BcExprType* type, bool posix_rel)
{
  BcStatus status;
  BcExpr expr;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) {
    return status;
  }

  if (parse->token.type != BC_LEX_LEFT_PAREN) {

    expr.type = BC_EXPR_SCALE;
    *type = BC_EXPR_SCALE;

    return bc_vec_push(exs, &expr);
  }

  expr.type = BC_EXPR_SCALE_FUNC;
  *type = BC_EXPR_SCALE_FUNC;

  expr.exprs = malloc(sizeof(BcVec));

  if (!expr.exprs) {
    return BC_STATUS_MALLOC_FAIL;
  }

  status = bc_parse_expr(parse, expr.exprs, posix_rel);

  if (status) {
    goto parse_err;
  }

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) {
    goto parse_err;
  }

  if (parse->token.type != BC_LEX_RIGHT_PAREN) {
    status = BC_STATUS_PARSE_INVALID_TOKEN;
    goto parse_err;
  }

  status = bc_vec_push(exs, &expr);

  if (status) {
    goto parse_err;
  }

  return bc_lex_next(&parse->lex, &parse->token);

parse_err:

  free(expr.exprs);

  return status;
}

static BcStatus bc_parse_incdec(BcParse* parse, BcVec* exs, BcExprType* prev)
{
  BcStatus status;
  BcLexTokenType type;
  BcExpr expr;
  BcExprType etype;

  etype = *prev;

  if (etype == BC_EXPR_VAR || etype == BC_EXPR_ARRAY_ELEM ||
      etype == BC_EXPR_SCALE || etype == BC_EXPR_LAST ||
      etype == BC_EXPR_IBASE || etype == BC_EXPR_OBASE)
  {
    expr.type = parse->token.type == BC_LEX_OP_INC ?
                    BC_EXPR_INC_POST : BC_EXPR_DEC_POST;
    *prev = expr.type;

    status = bc_vec_push(exs, &expr);
  }
  else {

    expr.type = parse->token.type == BC_LEX_OP_INC ?
                    BC_EXPR_INC_PRE : BC_EXPR_DEC_PRE;
    *prev = expr.type;

    status = bc_lex_next(&parse->lex, &parse->token);

    if (status) {
      return status;
    }

    type = parse->token.type;

    if (type != BC_LEX_NAME && type != BC_LEX_KEY_SCALE &&
        type != BC_LEX_KEY_LAST && type != BC_LEX_KEY_IBASE &&
        type != BC_LEX_KEY_OBASE)
    {
      return BC_STATUS_PARSE_INVALID_TOKEN;
    }

    status = bc_vec_push(exs, &expr);
  }

  if (status) {
    return status;
  }

  return bc_lex_next(&parse->lex, &parse->token);
}

static BcStatus bc_parse_minus(BcParse* parse, BcVec* exs, BcVec* ops,
                               BcExprType* prev, bool rparen, uint32_t* nexprs)
{
  BcStatus status;
  BcLexTokenType type;
  BcExprType etype;

  etype = *prev;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) {
    return status;
  }

  type = parse->token.type;

  if (type != BC_LEX_NAME && type != BC_LEX_NUMBER &&
      type != BC_LEX_KEY_SCALE && type != BC_LEX_KEY_LAST &&
      type != BC_LEX_KEY_IBASE && type != BC_LEX_KEY_OBASE &&
      type != BC_LEX_LEFT_PAREN)
  {
    return BC_STATUS_PARSE_INVALID_TOKEN;
  }

  type = rparen || etype == BC_EXPR_NUMBER ||
              etype == BC_EXPR_VAR || etype == BC_EXPR_ARRAY_ELEM ||
              etype == BC_EXPR_SCALE || etype == BC_EXPR_LAST ||
              etype == BC_EXPR_IBASE || etype == BC_EXPR_OBASE ?
                  BC_LEX_OP_MINUS : BC_LEX_OP_NEGATE;

  *prev = BC_PARSE_TOKEN_TO_EXPR(type);

  if (type == BC_LEX_OP_MINUS) {
    status = bc_parse_operator(parse, exs, ops, type, nexprs, false);
  }
  else {

    // We can just push onto the op stack because this is the largest
    // precedence operator that gets pushed. Inc/dec does not.
    status = bc_vec_push(ops, &type);
  }

  return status;
}

static BcStatus bc_parse_string(BcParse* parse, BcVec* code) {

  BcStatus status;
  size_t len;

  len = parse->program->strings.len;

  status = bc_vec_push(&parse->program->strings, &parse->token.string);

  if (status) {
    return status;
  }

  status = bc_parse_pushIndex(code, len);

  if (status) {
    return status;
  }

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) {
    return status;
  }

  return bc_parse_semicolonListEnd(parse, code);
}

static BcStatus bc_parse_return(BcParse* parse, BcStmtList* list) {

  BcStatus status;
  BcStmt stmt;
  BcExpr expr;

  if (!BC_PARSE_FUNC(parse)) {
    return BC_STATUS_PARSE_INVALID_TOKEN;
  }

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) {
    return status;
  }

  if (parse->token.type != BC_LEX_NEWLINE &&
      parse->token.type != BC_LEX_SEMICOLON &&
      parse->token.type != BC_LEX_LEFT_PAREN &&
      (status = bc_posix_error(BC_STATUS_POSIX_RETURN_PARENS,
                               parse->lex.file, parse->lex.line, NULL)))
  {
     return status;
  }

  stmt.type = BC_STMT_RETURN;

  stmt.data.exprs = malloc(sizeof(BcVec));

  if (!stmt.data.exprs) {
    return BC_STATUS_MALLOC_FAIL;
  }

  if (parse->token.type == BC_LEX_NEWLINE ||
      parse->token.type == BC_LEX_SEMICOLON)
  {
    status = bc_vec_init(stmt.data.exprs, sizeof(BcExpr), bc_expr_free);

    if (status) {
      goto parse_err;
    }

    expr.type = BC_EXPR_NUMBER;
    expr.string = NULL;

    status = bc_vec_push(stmt.data.exprs, &expr);

    if (status) {
      goto push_err;
    }
  }
  else {

    status = bc_parse_expr(parse, stmt.data.exprs, false);

    if (status) {
      goto parse_err;
    }
  }

  status = bc_list_insert(list, &stmt);

  if (status) {
    goto push_err;
  }

  return status;

push_err:

  bc_vec_free(stmt.data.exprs);

parse_err:

  free(stmt.data.exprs);

  return status;
}

static BcStatus bc_parse_print(BcParse* parse, BcStmtList* list) {

  BcStatus status;
  BcLexTokenType type;
  BcExpr expr;
  BcStmt stmt;
  bool comma;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) {
    return status;
  }

  type = parse->token.type;

  if (type == BC_LEX_SEMICOLON || type == BC_LEX_NEWLINE) {
    return BC_STATUS_PARSE_INVALID_PRINT;
  }

  comma = false;

  while (!status && type != BC_LEX_SEMICOLON && type != BC_LEX_NEWLINE) {

    if (type == BC_LEX_STRING) {

      status = bc_stmt_init(&stmt, BC_STMT_STRING_PRINT);

      if (status) {
        return status;
      }

      stmt.data.string = parse->token.string;

      status = bc_lex_next(&parse->lex, &parse->token);

      if (status) {
        return status;
      }
    }
    else {

      status =bc_stmt_init(&stmt, BC_STMT_EXPR);

      if (status) {
        return status;
      }

      status = bc_parse_expr(parse, stmt.data.exprs, false);

      if (status) {
        goto expr_err;
      }

      expr.type = BC_EXPR_PRINT;
      status = bc_vec_push(stmt.data.exprs, &expr);

      if (status) {
        goto parse_err;
      }
    }

    status = bc_list_insert(list, &stmt);

    if (status) {

      if (stmt.type == BC_STMT_EXPR) {
        goto parse_err;
      }

      return status;
    }

    if (parse->token.type == BC_LEX_COMMA) {
      comma = true;
      status = bc_lex_next(&parse->lex, &parse->token);
    }
    else {
      comma = false;
    }

    type = parse->token.type;
  }

  if (status) {
    return status;
  }

  if (comma) {
    return BC_STATUS_PARSE_INVALID_TOKEN;
  }

  return bc_lex_next(&parse->lex, &parse->token);

parse_err:

  bc_vec_free(stmt.data.exprs);

expr_err:

  free(stmt.data.exprs);

  return status;
}

static BcStatus bc_parse_if(BcParse* parse, BcStmtList* list) {

  BcStatus status;
  BcStmt stmt;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) {
    return status;
  }

  if (parse->token.type != BC_LEX_LEFT_PAREN) {
    return BC_STATUS_PARSE_INVALID_TOKEN;
  }

  stmt.type = BC_STMT_IF;
  stmt.data.if_stmt = bc_if_create();

  if (!stmt.data.if_stmt) {
    return BC_STATUS_MALLOC_FAIL;
  }

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) {
    goto parse_err;
  }

  status = bc_parse_expr(parse, &stmt.data.if_stmt->cond, true);

  if (status) {
    goto parse_err;
  }

  if (parse->token.type != BC_LEX_RIGHT_PAREN) {
    status = BC_STATUS_PARSE_INVALID_TOKEN;
    goto parse_err;
  }

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) {
    goto parse_err;
  }

  status = bc_parse_startBody(parse, &stmt.data.if_stmt->then_list,
                              BC_PARSE_FLAG_IF);

  if (status) {
    goto parse_err;
  }

  status = bc_list_insert(list, &stmt);

  if (status) {
    goto parse_err;
  }

  return status;

parse_err:

  free(stmt.data.if_stmt);

  return status;
}

static BcStatus bc_parse_while(BcParse* parse, BcStmtList* list) {

  BcStatus status;
  BcStmt stmt;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) {
    return status;
  }

  if (parse->token.type != BC_LEX_LEFT_PAREN) {
    return BC_STATUS_PARSE_INVALID_TOKEN;
  }

  stmt.type = BC_STMT_WHILE;
  stmt.data.while_stmt = bc_while_create();

  if (!stmt.data.while_stmt) {
    return BC_STATUS_MALLOC_FAIL;
  }

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) {
    goto parse_err;
  }

  status = bc_parse_expr(parse, &stmt.data.while_stmt->cond, true);

  if (status) {
    goto parse_err;
  }

  if (parse->token.type != BC_LEX_RIGHT_PAREN) {
    status = BC_STATUS_PARSE_INVALID_TOKEN;
    goto parse_err;
  }

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) {
    goto parse_err;
  }

  status = bc_parse_startBody(parse, &stmt.data.while_stmt->body,
                              BC_PARSE_FLAG_WHILE_LOOP);

  if (status) {
    goto parse_err;
  }

  status = bc_list_insert(list, &stmt);

  if (status) {
    goto parse_err;
  }

  return status;

parse_err:

  free(stmt.data.while_stmt);

  return status;
}

static BcStatus bc_parse_for(BcParse* parse, BcStmtList* list) {

  BcStatus status;
  BcStmt stmt;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) {
    return status;
  }

  if (parse->token.type != BC_LEX_LEFT_PAREN) {
    return BC_STATUS_PARSE_INVALID_TOKEN;
  }

  stmt.type = BC_STMT_FOR;
  stmt.data.for_stmt = bc_for_create();

  if (!stmt.data.for_stmt) {
    return BC_STATUS_MALLOC_FAIL;
  }

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) {
    goto parse_err;
  }

  if (parse->token.type != BC_LEX_SEMICOLON) {
    status = bc_parse_expr(parse, &stmt.data.for_stmt->init, false);
  }
  else {

    if ((status = bc_posix_error(BC_STATUS_POSIX_MISSING_FOR_INIT,
                                 parse->lex.file, parse->lex.line, NULL)))
    {
      goto parse_err;
    }

    status = bc_vec_init(&stmt.data.for_stmt->init,
                           sizeof(BcExpr), bc_expr_free);
  }

  if (status) {
    goto parse_err;
  }

  if (parse->token.type != BC_LEX_SEMICOLON) {
    status = BC_STATUS_PARSE_INVALID_TOKEN;
    goto init_err;
  }

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) {
    goto init_err;
  }

  if (parse->token.type != BC_LEX_SEMICOLON) {
    status = bc_parse_expr(parse, &stmt.data.for_stmt->cond, true);
  }
  else {

    if ((status = bc_posix_error(BC_STATUS_POSIX_MISSING_FOR_COND,
                                 parse->lex.file, parse->lex.line, NULL)))
    {
      goto init_err;
    }

    status = bc_vec_init(&stmt.data.for_stmt->cond,
                           sizeof(BcExpr), bc_expr_free);
  }

  if (status) {
    goto init_err;
  }

  if (parse->token.type != BC_LEX_SEMICOLON) {
    status = BC_STATUS_PARSE_INVALID_TOKEN;
    goto cond_err;
  }

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) {
    goto cond_err;
  }

  if (parse->token.type != BC_LEX_RIGHT_PAREN) {
    status = bc_parse_expr(parse, &stmt.data.for_stmt->update, false);
  }
  else {

    if ((status = bc_posix_error(BC_STATUS_POSIX_MISSING_FOR_UPDATE,
                                 parse->lex.file, parse->lex.line, NULL)))
    {
      goto cond_err;
    }

    status = bc_vec_init(&stmt.data.for_stmt->update,
                           sizeof(BcExpr), bc_expr_free);
  }

  if (status) {
    goto cond_err;
  }

  if (parse->token.type != BC_LEX_RIGHT_PAREN) {
    status = BC_STATUS_PARSE_INVALID_TOKEN;
    goto update_err;
  }

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) {
    goto update_err;
  }

  status = bc_parse_startBody(parse, &stmt.data.for_stmt->body,
                              BC_PARSE_FLAG_FOR_LOOP);

  if (status) {
    goto update_err;
  }

  status = bc_list_insert(list, &stmt);

  if (status) {
    goto update_err;
  }

  return status;

update_err:

  bc_vec_free(&stmt.data.for_stmt->update);

cond_err:

  bc_vec_free(&stmt.data.for_stmt->cond);

init_err:

  bc_vec_free(&stmt.data.for_stmt->init);

parse_err:

  free(stmt.data.for_stmt);

  return status;
}

static BcStatus bc_parse_startBody(BcParse* parse, BcVec** code, uint8_t flags)
{
  BcStatus status;
  uint8_t* flag_ptr;

  status = bc_vec_init(*code, sizeof(uint8_t), NULL);

  if (status) {
    return status;
  }

  if (parse->token.type == BC_LEX_LEFT_BRACE) {

    status = bc_vec_push(&parse->ctx_stack, code);

    if (status) {
      goto parse_err;
    }

    flag_ptr = bc_vec_top(&parse->flag_stack);

    if (!flag_ptr) {
      status = BC_STATUS_PARSE_BUG;
      goto parse_err;
    }

    flags |= (*flag_ptr & (BC_PARSE_FLAG_FUNC | BC_PARSE_FLAG_FOR_LOOP |
                           BC_PARSE_FLAG_WHILE_LOOP));

    status = bc_vec_push(&parse->flag_stack, &flags);

    if (status) {
      goto parse_err;
    }

    ++parse->num_braces;

    status = bc_lex_next(&parse->lex, &parse->token);

    if (status) {
      goto parse_err;
    }
  }
  else {

    while (parse->token.type == BC_LEX_NEWLINE) {

      status = bc_lex_next(&parse->lex, &parse->token);

      if (status) {
        goto parse_err;
      }
    }
  }

  status = bc_parse_stmt(parse, *code);

  if (status && status != BC_STATUS_LEX_EOF && status != BC_STATUS_PARSE_EOF) {
    goto parse_err;
  }

  return status;

parse_err:

  bc_vec_free(*code);
  *code = NULL;

  return status;
}

static BcStatus bc_parse_loopExit(BcParse* parse, BcStmtList* list,
                                  BcLexTokenType type)
{
  BcStatus status;
  BcStmt stmt;
  BcStmtType stype;

  if (!BC_PARSE_FOR_LOOP(parse) && !BC_PARSE_WHILE_LOOP(parse)) {
    return BC_STATUS_PARSE_INVALID_TOKEN;
  }

  stype = type == BC_LEX_KEY_BREAK ? BC_STMT_BREAK : BC_STMT_CONTINUE;

  status = bc_stmt_init(&stmt, stype);

  if (status) {
    return status;
  }

  status = bc_list_insert(list, &stmt);

  if (status) {
    bc_stmt_free(&stmt);
    return status;
  }

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) {
    return status;
  }

  if (parse->token.type != BC_LEX_SEMICOLON &&
      parse->token.type != BC_LEX_NEWLINE)
  {
    return BC_STATUS_PARSE_INVALID_TOKEN;
  }

  return bc_lex_next(&parse->lex, &parse->token);
}

static BcStatus bc_parse_rightBrace(BcParse* parse) {

  BcStatus status;
  BcStmtList** list_ptr;
  BcStmtList* top;
  BcStmt* if_stmt;

  if (parse->ctx_stack.len <= 1 || parse->num_braces == 0) {
    return BC_STATUS_PARSE_INVALID_TOKEN;
  }

  if (parse->ctx_stack.len != parse->flag_stack.len) {
    return BC_STATUS_PARSE_BUG;
  }

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) {
    return status;
  }

  if (BC_PARSE_IF(parse)) {

    while (parse->token.type == BC_LEX_NEWLINE) {

      status = bc_lex_next(&parse->lex, &parse->token);

      if (status) {
        return status;
      }
    }

    if (parse->token.type == BC_LEX_KEY_ELSE) {

      status = bc_vec_pop(&parse->flag_stack);

      if (status) {
        return status;
      }

      status = bc_vec_pop(&parse->ctx_stack);

      if (status) {
        return status;
      }

      status = bc_lex_next(&parse->lex, &parse->token);

      if (status) {
        return status;
      }

      top = *(BC_PARSE_TOP_CTX(parse));
      if_stmt = bc_list_last(top);
      list_ptr = &if_stmt->data.if_stmt->else_list;

      return bc_parse_startBody(parse, list_ptr, BC_PARSE_FLAG_ELSE);
    }
  }
  else if (BC_PARSE_FUNC_INNER(parse)) {
    parse->func = NULL;
  }

  status = bc_vec_pop(&parse->flag_stack);

  if (status) {
    return status;
  }

  return bc_vec_pop(&parse->ctx_stack);
}

static BcStatus bc_parse_pushNum(BcVec* code, const char* num) {

  BcStatus status;
  size_t len;

  status = BC_STATUS_SUCCESS;
  len = strlen(num);

  for (size_t i = 0; !status && i < len; ++i) {
    status = bc_vec_pushByte(code, (uint8_t) num[i]);
  }

  if (status) {
    return status;
  }

  return bc_vec_pushByte(code, (uint8_t) ':');
}

static BcStatus bc_parse_pushIndex(BcVec* code, size_t idx) {

  BcStatus status;
  uint8_t amt;
  uint8_t nums[sizeof(size_t)];
  uint32_t i;

  amt = 0;
  i = 0;

  while (idx) {
    nums[amt] = (uint8_t) idx;
    idx &= ~(0xFF);
    idx >>= sizeof(uint8_t);
    ++amt;
  }

  status = bc_vec_pushByte(code, amt);

  if (status) {
    return status;
  }

  while (!status && i < amt) {
    status = bc_vec_pushByte(code, nums[idx]);
    ++i;
  }

  return status;
}
