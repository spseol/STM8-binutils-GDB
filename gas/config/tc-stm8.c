/* tc-stm8.c -- Assembler for the STM8.
   Written by Ake Rehnman 2017-02-21,
   ake.rehnman (at) gmail dot com

   Copyright (C) 2007-2017 Free Software Foundation, Inc.

   This file is part of BFD, the Binary File Descriptor library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.  */

#include "as.h"
#include "opcode/stm8.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "opcode/stm8.h"
#include <string.h>

typedef enum
{
  OP_ILLEGAL = 0,
  OP_IMM,
  OP_SHORTMEM,
  OP_MEM,
  OP_INDX,
  OP_INDY,
  OP_SOFF_X,
  OP_OFF_X,
  OP_SOFF_Y,
  OP_OFF_Y,
  OP_SOFF_SP,
  OP_SPTRW,
  OP_LPTRW,
  OP_LPTRE,
  OP_SPTRW_X,
  OP_LPTRW_X,
  OP_SPTRW_Y,
  OP_LPTRW_Y,
  OP_LPTRE_X,
  OP_LPTRE_Y,
  OP_REGISTER,
  OP_HI8,
  OP_LO8,
  OP_HH8
} stm8_operand_t;

typedef struct
{
  /* Name of the expression modifier allowed with .byte, .word, etc.  */
  const char *name;

  /* Only allowed with n bytes of data.  */
  int nbytes;

  /* Associated RELOC.  */
  bfd_reloc_code_real_type reloc;

  /* Part of the error message.  */
  const char *error;

  /* Internal operand. */
  stm8_operand_t op;

} exp_mod_data_t;

static htab_t stm8_hash;

const char comment_chars[] = ";";
const char line_comment_chars[] = "#";
const char line_separator_chars[] = "{";

int md_short_jump_size = 3;
int md_long_jump_size = 4;

#ifdef DEBUG_STM8
int stm8_debug_dump = 0;
#endif

/* The target specific pseudo-ops which we support.  */
/* example:
const pseudo_typeS md_pseudo_table[] =
{
  {"arch", avr_set_arch,  0},
  { NULL, NULL,   0}
};
*/
const pseudo_typeS md_pseudo_table[] = { { NULL, NULL, 0 } };

const char EXP_CHARS[] = "eE";

/* Chars that mean this number is a floating point constant.
   As in 0f12.456
   or    0d1.2345e12  */
const char FLT_CHARS[] = "rRsSfFdDxXpP";

/* On the Z8000, a PC-relative offset is relative to the address of the
   instruction plus its size.  */
long
md_pcrel_from (fixS *fixP)
{
  return fixP->fx_size + fixP->fx_where + fixP->fx_frag->fr_address;
}

const char *
md_atof (int type, char *litP, int *sizeP)
{
  return ieee_md_atof (type, litP, sizeP, TRUE);
}

void
md_show_usage (FILE *stream)
{
  fprintf (stream, _ ("\
  STM8 options:\n\
  "));
#ifdef DEBUG_STM8
  fprintf (stream, _ ("\
  --debug               turn on debug messages\n"));
#endif
}

const char *md_shortopts = "";

struct option md_longopts[] = {
#ifdef DEBUG_STM8
  { "debug", no_argument, &stm8_debug_dump, 1 },
#endif
  { NULL, no_argument, NULL, 0 }
};

size_t md_longopts_size = sizeof (md_longopts);

int
md_parse_option (int c __attribute__ ((unused)),
                 const char *arg __attribute__ ((unused)))
{
  return 1;
}

void
md_begin (void)
{
  const struct stm8_opcodes_s *opcode;

  stm8_hash = str_htab_create ();

  /* Insert unique names into hash table.  This hash table then provides a
  quick index to the first opcode with a particular name in the opcode
  table.  */
  for (opcode = stm8_opcodes; opcode->name; opcode++)
    str_hash_insert (stm8_hash, opcode->name, (char *)opcode, 0);

  // add register names to symbol table

  symbol_table_insert (
      symbol_create ("A", reg_section, &zero_address_frag, ST8_REG_A));

  symbol_table_insert (
      symbol_create ("X", reg_section, &zero_address_frag, ST8_REG_X));

  symbol_table_insert (
      symbol_create ("Y", reg_section, &zero_address_frag, ST8_REG_Y));

  symbol_table_insert (
      symbol_create ("SP", reg_section, &zero_address_frag, ST8_REG_SP));

  symbol_table_insert (
      symbol_create ("CC", reg_section, &zero_address_frag, ST8_REG_CC));

  symbol_table_insert (
      symbol_create ("XL", reg_section, &zero_address_frag, ST8_REG_XL));

  symbol_table_insert (
      symbol_create ("XH", reg_section, &zero_address_frag, ST8_REG_XH));

  symbol_table_insert (
      symbol_create ("YL", reg_section, &zero_address_frag, ST8_REG_YL));

  symbol_table_insert (
      symbol_create ("YH", reg_section, &zero_address_frag, ST8_REG_YH));

  symbol_table_insert (
      symbol_create ("a", reg_section, &zero_address_frag, ST8_REG_A));

  symbol_table_insert (
      symbol_create ("x", reg_section, &zero_address_frag, ST8_REG_X));

  symbol_table_insert (
      symbol_create ("y", reg_section, &zero_address_frag, ST8_REG_Y));

  symbol_table_insert (
      symbol_create ("sp", reg_section, &zero_address_frag, ST8_REG_SP));

  symbol_table_insert (
      symbol_create ("cc", reg_section, &zero_address_frag, ST8_REG_CC));

  symbol_table_insert (
      symbol_create ("xl", reg_section, &zero_address_frag, ST8_REG_XL));

  symbol_table_insert (
      symbol_create ("xh", reg_section, &zero_address_frag, ST8_REG_XH));

  symbol_table_insert (
      symbol_create ("yl", reg_section, &zero_address_frag, ST8_REG_YL));

  symbol_table_insert (
      symbol_create ("yh", reg_section, &zero_address_frag, ST8_REG_YH));
}

const exp_mod_data_t exp_mod_data[] = {
  /* Default, must be first.  */
  { "", 0, BFD_RELOC_16, "", OP_ILLEGAL },
  { "lo8", 1, BFD_RELOC_STM8_LO8, "`lo8' ", OP_LO8 },
  { "hi8", 1, BFD_RELOC_STM8_HI8, "`hi8' ", OP_HI8 },
  { "hh8", 1, BFD_RELOC_STM8_HH8, "`hi8' ", OP_HH8 },
};

static inline char *
skip_space (char *s)
{
  while (*s == ' ' || *s == '\t')
    ++s;
  return s;
}

/* Extract one word from FROM and copy it to TO.  */

static char *
extract_word (char *from, char *to, int limit)
{
  char *op_end;
  int size = 0;

  /* Drop leading whitespace.  */
  from = skip_space (from);
  *to = 0;

  /* Find the op code end.  */
  for (op_end = from; *op_end != 0 && is_part_of_name (*op_end);)
    {
      to[size++] = *op_end++;
      if (size + 1 >= limit)
        break;
    }

  to[size] = 0;
  return op_end;
}

void
md_operand (expressionS *exp __attribute__ ((unused)))
{
}

/* Attempt to simplify or eliminate a fixup. To indicate that a fixup
   has been eliminated, set fix->fx_done. If fix->fx_addsy is non-NULL,
   we will have to generate a reloc entry.  */
void
md_apply_fix (fixS *fixP, valueT *valP, segT segment ATTRIBUTE_UNUSED)
{
  long val = *(long *)valP;
  char *buf = fixP->fx_where + fixP->fx_frag->fr_literal;

  DEBUG_TRACE_FIXUP (fixP);

  switch (fixP->fx_r_type)
    {
    case BFD_RELOC_8:
      if (fixP->fx_addsy)
        {
          fixP->fx_no_overflow = 1;
          fixP->fx_done = 0;
        }
      else
        *buf++ = val;
      break;

    case BFD_RELOC_16:
      if (fixP->fx_addsy)
        {
          fixP->fx_no_overflow = 1;
          fixP->fx_done = 0;
        }
      else
        {
          *buf++ = (val >> 8);
          *buf++ = val;
        }
      break;

    case BFD_RELOC_24:
      if (fixP->fx_addsy)
        {
          fixP->fx_no_overflow = 1;
          fixP->fx_done = 0;
          fixP->fx_where--;
        }
      else
        {
          *buf++ = (val >> 16);
          *buf++ = (val >> 8);
          *buf++ = val;
        }
      break;

    case BFD_RELOC_32:
      if (fixP->fx_addsy)
        {
          fixP->fx_no_overflow = 1;
          fixP->fx_done = 0;
        }
      else
        {
          *buf++ = (val >> 24);
          *buf++ = (val >> 16);
          *buf++ = (val >> 8);
          *buf++ = val;
        }
      break;

    case BFD_RELOC_8_PCREL:
      if (fixP->fx_addsy)
        {
          fixP->fx_pcrel_adjust = -1;
          fixP->fx_no_overflow = 1;
          fixP->fx_done = 0;
        }
      else
        {
          if (val > 127 || val < -128)
            as_bad_where (fixP->fx_file, fixP->fx_line,
                          _ ("relative jump out of range"));
          *buf++ = val;
          fixP->fx_no_overflow = 1;
          fixP->fx_done = 1;
        }
      break;

    case BFD_RELOC_STM8_BIT_FLD:
      if (val > 7 || val < 0)
        as_bad_where (fixP->fx_file, fixP->fx_line,
                      _ ("bitfield out of range %ld"), val);
      *buf++ += val * 2;
      fixP->fx_no_overflow = 1;
      fixP->fx_done = 1;
      break;

    case BFD_RELOC_STM8_LO8:
      fixP->fx_no_overflow = 1;
      if (!fixP->fx_addsy)
        *buf = 0xff & val;
      break;

    case BFD_RELOC_STM8_HI8:
      fixP->fx_no_overflow = 1;
      if (!fixP->fx_addsy)
        *buf = (0xff00 & val) >> 8;
      break;

    case BFD_RELOC_STM8_HH8:
      fixP->fx_no_overflow = 1;
      if (!fixP->fx_addsy)
        *buf = (0xff0000 & val) >> 16;
      break;

    default:
      printf (_ ("md_apply_fix: unknown r_type 0x%x\n"), fixP->fx_r_type);
      abort ();
    }

  if (fixP->fx_addsy == NULL && fixP->fx_pcrel == 0)
    fixP->fx_done = 1;
}

/* Generate a machine dependent reloc from a fixup.  */

arelent *
tc_gen_reloc (asection *section ATTRIBUTE_UNUSED, fixS *fixp ATTRIBUTE_UNUSED)
{
  arelent *reloc;

  DEBUG_TRACE_FIXUP (fixp);

  reloc = XNEW (arelent);
  reloc->sym_ptr_ptr = XNEW (asymbol *);
  *reloc->sym_ptr_ptr = symbol_get_bfdsym (fixp->fx_addsy);
  reloc->address = fixp->fx_frag->fr_address + fixp->fx_where;
  reloc->addend = fixp->fx_offset;
  if (fixp->fx_r_type == BFD_RELOC_8_PCREL)
    reloc->addend = fixp->fx_offset + fixp->fx_pcrel_adjust;
  reloc->howto = bfd_reloc_type_lookup (stdoutput, fixp->fx_r_type);

  if (!reloc->howto)
    {
      as_bad_where (fixp->fx_file, fixp->fx_line,
                    _ ("Cannot represent %s relocation in object file"),
                    bfd_get_reloc_code_name (fixp->fx_r_type));
      abort ();
    }
  return reloc;
}

valueT
md_section_align (segT seg, valueT size)
{
  int align = bfd_section_alignment (seg);
  valueT mask = ((valueT)1 << align) - 1;

  return (size + mask) & ~mask;
}

symbolS *
md_undefined_symbol (char *name)
{
  return 0;
  // Hande the case where a symbol has .w or .e suffix attached.
  // This is actually quite stupid because the operand length is the
  // same independent of whether it is .w or .e
  char *p;
  symbolS *symbolP = NULL;

  if ((p = strstr (name, ".w")))
    {
      if (*(p + 2) == 0)
        {
          *p = 0;
          symbolP = symbol_find (name);
          //*p = '.';
        }
    }
  if ((p = strstr (name, ".l")))
    {
      if (*(p + 2) == 0)
        {
          *p = 0;
          symbolP = symbol_find (name);
          //*p = '.';
        }
    }
  if ((p = strstr (name, ".e")))
    {
      if (*(p + 2) == 0)
        {
          *p = 0;
          symbolP = symbol_find (name);
          //*p = '.';
        }
    }
  return symbolP;
}

void
md_create_long_jump (char *ptr __attribute__ ((unused)),
                     addressT from_addr ATTRIBUTE_UNUSED,
                     addressT to_addr __attribute__ ((unused)),
                     fragS *frag __attribute__ ((unused)),
                     symbolS *to_symbol __attribute__ ((unused)))
{
  as_bad (_ ("long_jump"));
}

void
md_create_short_jump (char *ptr __attribute__ ((unused)),
                      addressT from_addr __attribute__ ((unused)),
                      addressT to_addr ATTRIBUTE_UNUSED,
                      fragS *frag ATTRIBUTE_UNUSED,
                      symbolS *to_symbol ATTRIBUTE_UNUSED)
{
  as_bad (_ ("short_jump"));
}

/* Put number into target byte order.  */

void
md_number_to_chars (char *ptr, valueT use, int nbytes)
{
  number_to_chars_bigendian (ptr, use, nbytes);
}

int
md_estimate_size_before_relax (fragS *fragP ATTRIBUTE_UNUSED,
                               segT segment_type ATTRIBUTE_UNUSED)
{
  as_bad (_ ("call to md_estimate_size_before_relax\n"));
  return 0;
}

void
md_convert_frag (bfd *abfd ATTRIBUTE_UNUSED, segT sec ATTRIBUTE_UNUSED,
                 fragS *fragP ATTRIBUTE_UNUSED)
{
  as_bad (_ ("call to md_convert_frag\n"));
}

static char *
match_parentheses (char *str)
{
  char *p;
  int cnt = 0;

  p = str;
  while (*p != 0)
    {
      if (*p == '(')
        cnt++;
      if (*p == ')')
        {
          if (--cnt == 0)
            return p;
        }
      p++;
    }
  return 0;
}

static int
split_words (char *str, char **chunks)
{
  int i;
  char *p;

  p = str;
  for (i = 0; i < 3; i++)
    {
      chunks[i] = str;
      if (*str == 0)
        break;
      while (*p != 0)
        {
          if (*p == '(')
            {
              p = match_parentheses (str);
              if (p == 0)
                return 0;
            }
          if (*p == ',')
            {
              *p = 0;
              p++;
              break;
            }
          p++;
        }
      str = p;
    }
  return i;
}

static int
read_arg_ptr (char *str, expressionS *exps)
{
  char *s;
  char *p;
  char c;

  if ((str[0] == '[') && (strstr (str, "]")))
    {
      s = str;
      s++;
      input_line_pointer = s;

      /* first eat up .s .w and .e */
      if ((p = strstr (s, ".s]")))
        {
          c = *p;
          *p = 0;
        }
      else if ((p = strstr (s, ".w]")))
        {
          c = *p;
          *p = 0;
        }
      else if ((p = strstr (s, ".e]")))
        {
          c = *p;
          *p = 0;
        }

      expression (exps);
      DEBUG_TRACE_EXPR (exps);

      /* restore c */
      if (p)
        *p = c;

      // return default pointer len
      if (*input_line_pointer == ']')
        {
          input_line_pointer += 1;
          return 2;
        }
      if ((*input_line_pointer == '.') && (*(input_line_pointer + 1) == 's'))
        {
          input_line_pointer += 2;
          return 1;
        }
      else if ((*input_line_pointer == '.')
               && (*(input_line_pointer + 1) == 'w'))
        {
          input_line_pointer += 2;
          return 2;
        }
      else if ((*input_line_pointer == '.')
               && (*(input_line_pointer + 1) == 'e'))
        {
          input_line_pointer += 2;
          return 3;
        }
      else
        {
          as_bad ("Expected ']' or '.s' or .e' or '.w' but found '%c'",
                  *input_line_pointer);
          return -1;
        }
    }
  return 0;
}

static int
read_arg_idx (char *str, expressionS *exps)
{
  char *s;
  char *p;
  char c;

  s = str;
  input_line_pointer = s;

  /* first eat up .s */
  if ((p = strstr (s, ".s")))
    {
      c = *p;
      *p = 0;
    }

  expression (exps);
  DEBUG_TRACE_EXPR (exps);

  /* restore c */
  if (p)
    *p = c;

  // return default offset len
  if (*input_line_pointer == ',')
    {
      input_line_pointer += 1;
      return 2;
    }
  else if ((*input_line_pointer == '.') && (*(input_line_pointer + 1) == 's'))
    {
      input_line_pointer += 2;
      return 1;
    }
  else
    {
      as_bad ("Expected ',' or '.s' but found '%c'", *input_line_pointer);
      return -1;
    }

  return 0;
}

char *toupperstr (char *str);

char *
toupperstr (char *str)
{
  int i;
  for (i = 0; str[i]; i++)
    {
      str[i] = toupper (str[i]);
    }
  return str;
}

char *strend (const char *str, const char *cmp);

char *
strend (const char *str, const char *cmp)
{
  str = strrchr (str, cmp[0]);

  if (str != NULL)
    if (!strcmp (str, cmp))
      return (char *)str;

  return (0);
}

/* In: argument
   Out: value
   Modifies: type */
static int
read_arg (char *str, expressionS *exps)
{
  int ret;
  const char *name ATTRIBUTE_UNUSED;
  /* There is a number of addressing modes in ST8 architecture.
We need to properly handle each of them in order to find a proper opcode. */
  if (!str)
    return (0);

  // Immidate
  if (str[0] == '#')
    {
      str++;
      exps->X_md = OP_IMM;
      input_line_pointer = str;
      expression (exps);
      DEBUG_TRACE_EXPR (exps);
      return 1;
    }

  char strx[256];
  strncpy (strx, str, sizeof (strx) - 1);
  toupperstr (strx);

  // decode ptr operand
  if (str[0] == '[')
    {
      ret = read_arg_ptr (str, exps);
      if (ret > 0)
        {
          if (ret == 1)
            {
              exps->X_md = OP_SPTRW;
              return 1;
            }
          if (ret == 2)
            {
              exps->X_md = OP_LPTRW;
              return 1;
            }
          if (ret == 3)
            {
              exps->X_md = OP_LPTRE;
              return 1;
            }
        }
      else
        return 0;
    }
  // decode index operands
  // index X
  else if ((str[0] == '(') && (strstr (strx, "(X)")))
    {
      exps->X_md = OP_INDX;
      return 1;
    }
  // index Y
  else if ((str[0] == '(') && (strstr (strx, "(Y)")))
    {
      exps->X_md = OP_INDY;
      return 1;
    }
  // offset,X
  else if ((str[0] == '(') && (strstr (strx, ",X)")))
    {
      str++;
      if (str[0] == '[')
        {
          ret = read_arg_ptr (str, exps);
          if (ret == 1)
            {
              exps->X_md = OP_SPTRW_X;
              return 1;
            }
          if (ret == 2)
            {
              exps->X_md = OP_LPTRW_X;
              return 1;
            }
          if (ret == 3)
            {
              exps->X_md = OP_LPTRE_X;
              return 1;
            }
        }

      else
        {
          ret = read_arg_idx (str, exps);
          if (ret == 1)
            {
              exps->X_md = OP_SOFF_X;
              return 1;
            }
          if (ret == 2)
            {
              exps->X_md = OP_OFF_X;
              return 1;
            }
        }
      return 0;
    }
  // offset,Y
  else if ((str[0] == '(') && (strstr (strx, ",Y)")))
    {
      str++;
      if (str[0] == '[')
        {
          ret = read_arg_ptr (str, exps);
          if (ret == 1)
            {
              exps->X_md = OP_SPTRW_Y;
              return 1;
            }
          if (ret == 2)
            {
              exps->X_md = OP_LPTRW_Y;
              return 1;
            }
          if (ret == 3)
            {
              exps->X_md = OP_LPTRE_Y;
              return 1;
            }
        }
      else
        {
          ret = read_arg_idx (str, exps);
          if (ret == 1)
            {
              exps->X_md = OP_SOFF_Y;
              return 1;
            }
          if (ret == 2)
            {
              exps->X_md = OP_OFF_Y;
              return 1;
            }
        }
      return 0;
    }
  // offset,SP
  else if ((str[0] == '(') && (strstr (strx, ",SP)")))
    {
      str++;
      ret = read_arg_idx (str, exps);
      if (ret > 0)
        {
          exps->X_md = OP_SOFF_SP;
          return 1;
        }
      return 0;
    }
  else
    {
      /* The first entry of exp_mod_data[] contains an entry if no
        expression modifier is present.  Skip it.  */
      size_t i;

      for (i = 1; i < ARRAY_SIZE (exp_mod_data); i++)
        {
          const exp_mod_data_t *const pexp = &exp_mod_data[i];
          const size_t len = strlen (pexp->name);
          const int result = strncasecmp (str, pexp->name, len);

          if (!result)
            {
              str += len;
              while (isspace ((int)*str))
                str++;

              if (*str == '(')
                {
                  input_line_pointer = ++str;

                  expression (exps);

                  if (*input_line_pointer == ')')
                    {
                      input_line_pointer++;

                      exps->X_md = pexp->op;

                      if (*input_line_pointer)
                        {
                          as_bad (_ ("garbage in operand '%s'"),
                                  input_line_pointer);
                          return 0;
                        }

                      return 1;
                    }
                  else
                    {
                      as_bad (_ ("`)' required"));
                      return 0;
                    }
                }

              break;
            }
        }
    }

  char *p;
  if ((p = strend (str, ".s")))
    {
      *p = 0;
      exps->X_md = OP_SHORTMEM;
      input_line_pointer = str;
      expression (exps);
      DEBUG_TRACE_EXPR (exps);
      return 1;
    }

  input_line_pointer = str;
  expression (exps);
  DEBUG_TRACE_EXPR (exps);

  if (exps->X_op == O_register)
    {
      exps->X_md = OP_REGISTER;
      return 1;
    }

  if (exps->X_op != O_illegal)
    {
      exps->X_md = OP_MEM;
      return 1;
    }

  /* Can't parse an expression, notifying caller about that. */
  return (0);
}

static int
read_args (char *str, expressionS exps[])
{
  char *chunks[3];
  int count = split_words (str, chunks);
  int i;
  for (i = 0; i < count; i++)
    {
      int ret = read_arg (chunks[i], &(exps[i]));
      if (!ret)
        as_bad ("Invalid operand: %s", chunks[i]);
    }
  return (count);
}

static void
stm8_bfd_out (struct stm8_opcodes_s op, expressionS exp[], int count,
              char *frag)
{
  int i;
  int arg = 0;
  int dir = 1;

  /* if count is negative the arguments are reversed */
  if (count < 0)
    {
      count = -count;
      arg = count - 1;
      dir = -1;
    }

  for (i = 0; i < count; i++, arg += dir)
    {
      int where = frag - frag_now->fr_literal;

      if (exp[arg].X_op != O_illegal)
        {
          switch (op.constraints[arg])
            {
            case ST8_REG_CC:
            case ST8_REG_A:
            case ST8_REG_X:
            case ST8_REG_Y:
            case ST8_REG_SP:
            case ST8_REG_XL:
            case ST8_REG_XH:
            case ST8_REG_YL:
            case ST8_REG_YH:
            case ST8_INDX:
            case ST8_INDY:
              break;
            case ST8_EXTMEM:
            case ST8_EXTOFF_X:
            case ST8_EXTOFF_Y:
              fix_new_exp (frag_now, where, 3, &exp[arg], FALSE, BFD_RELOC_24);
              bfd_put_bits (0xaaaaaaaa, frag, 24, true);
              frag += 3;
              break;
            case ST8_LONGPTRW_Y:
            case ST8_LONGPTRW_X:
            case ST8_LONGPTRW:
            case ST8_LONGPTRE_Y:
            case ST8_LONGPTRE_X:
            case ST8_LONGPTRE:
            case ST8_LONGOFF_Y:
            case ST8_LONGOFF_X:
            case ST8_WORD:
            case ST8_LONGMEM:
              fix_new_exp (frag_now, where, 2, &exp[arg], FALSE, BFD_RELOC_16);
              bfd_put_bits (0xaaaaaaaa, frag, 16, true);
              frag += 2;
              break;
            case ST8_SHORTPTRW_Y:
            case ST8_SHORTPTRW_X:
            case ST8_SHORTPTRW:
            case ST8_SHORTOFF_Y:
            case ST8_SHORTOFF_X:
            case ST8_SHORTOFF_SP:
            case ST8_BYTE:
            case ST8_SHORTMEM:
              if (exp[arg].X_md == OP_LO8)
                fix_new_exp (frag_now, where, 1, &exp[arg], FALSE,
                             BFD_RELOC_STM8_LO8);
              else if (exp[arg].X_md == OP_HI8)
                fix_new_exp (frag_now, where, 1, &exp[arg], FALSE,
                             BFD_RELOC_STM8_HI8);
              else if (exp[arg].X_md == OP_HH8)
                fix_new_exp (frag_now, where, 1, &exp[arg], FALSE,
                             BFD_RELOC_STM8_HH8);
              else
                fix_new_exp (frag_now, where, 1, &exp[arg], FALSE,
                             BFD_RELOC_8);
              bfd_put_bits (0xaaaaaaaa, frag, 8, true);
              frag += 1;
              break;
            case ST8_PCREL:
              fix_new_exp (frag_now, where, 1, &exp[arg], TRUE,
                           BFD_RELOC_8_PCREL);
              bfd_put_bits (0xaaaaaaaa, frag, 8, true);
              frag += 1;
              break;
            case ST8_BIT_0:
            case ST8_BIT_1:
            case ST8_BIT_2:
            case ST8_BIT_3:
            case ST8_BIT_4:
            case ST8_BIT_5:
            case ST8_BIT_6:
            case ST8_BIT_7:
              fix_new_exp (frag_now, where - 3, 1, &exp[arg], FALSE,
                           BFD_RELOC_STM8_BIT_FLD);
              break;
            case ST8_END:
              as_fatal (_ ("BUG: illigal op constraint"));
              break;
            }
        }
    }
}

static int
cmpspec (stm8_addr_mode_t addr_mode[], expressionS exps[], int count)
{
  int i, ret = 0;
  unsigned int value;
  stm8_operand_t operand;

  for (i = 0; i < count; i++)
    {
      operand = exps[i].X_md;
      if (!addr_mode || !operand)
        continue; // End
      if (exps[i].X_op == O_constant)
        value = exps[i].X_add_number;
      else
        value = -1;

      switch (operand)
        {
        case OP_REGISTER:
          if (addr_mode[i] == (stm8_addr_mode_t)exps[i].X_add_number)
            continue;
          break;
        case OP_IMM:
          if (addr_mode[i] == ST8_BYTE)
            continue;
          if (addr_mode[i] == ST8_WORD)
            continue;
          /* for bit immidiate operand it is important to only match the opcode
            with ST8_BIT_0 operand. We will later fix the insn bit value in
            BFD_RELOC_STM8_BIT_FLD */
          if (addr_mode[i] == ST8_BIT_0)
            continue;
          break;
        case OP_INDX:
          if (addr_mode[i] == ST8_INDX)
            continue;
          break;
        case OP_INDY:
          if (addr_mode[i] == ST8_INDY)
            continue;
          break;
        case OP_SOFF_X:
          if (addr_mode[i] == ST8_SHORTOFF_X)
            continue;
          break;
        case OP_OFF_X:
          if (addr_mode[i] == ST8_SHORTOFF_X)
            if (value < 0x100)
              continue;
          if (addr_mode[i] == ST8_LONGOFF_X)
            continue;
          if (addr_mode[i] == ST8_EXTOFF_X)
            continue;
          break;
        case OP_SOFF_Y:
          if (addr_mode[i] == ST8_SHORTOFF_Y)
            continue;
          break;
        case OP_OFF_Y:
          if (addr_mode[i] == ST8_SHORTOFF_Y)
            if (value < 0x100)
              continue;
          if (addr_mode[i] == ST8_LONGOFF_Y)
            continue;
          if (addr_mode[i] == ST8_EXTOFF_Y)
            continue;
          break;
        case OP_SOFF_SP:
          if (addr_mode[i] == ST8_SHORTOFF_SP)
            continue;
          break;
        case OP_SPTRW:
          if (addr_mode[i] == ST8_SHORTPTRW)
            continue;
          break;
        case OP_LPTRW:
          if (addr_mode[i] == ST8_LONGPTRW)
            continue;
          break;
        case OP_SPTRW_X:
          if (addr_mode[i] == ST8_SHORTPTRW_X)
            continue;
          break;
        case OP_LPTRW_X:
          if (addr_mode[i] == ST8_LONGPTRW_X)
            continue;
          break;
        case OP_SPTRW_Y:
          if (addr_mode[i] == ST8_SHORTPTRW_Y)
            continue;
          break;
        case OP_LPTRW_Y:
          if (addr_mode[i] == ST8_LONGPTRW_Y)
            continue;
          break;
        case OP_LPTRE:
          if (addr_mode[i] == ST8_LONGPTRE)
            continue;
          break;
        case OP_LPTRE_X:
          if (addr_mode[i] == ST8_LONGPTRE_X)
            continue;
          break;
        case OP_LPTRE_Y:
          if (addr_mode[i] == ST8_LONGPTRE_Y)
            continue;
          break;
        case OP_MEM:
          if (addr_mode[i] == ST8_PCREL)
            continue;
          if (addr_mode[i] == ST8_EXTMEM)
            continue;
          if (addr_mode[i] == ST8_LONGMEM)
            continue;
          break;
        case OP_SHORTMEM:
          if (addr_mode[i] == ST8_SHORTMEM)
            continue;
          break;
        case OP_LO8:
        case OP_HI8:
        case OP_HH8:
          if (addr_mode[i] == ST8_BYTE)
            continue;
          break;
        case OP_ILLEGAL:
          as_fatal (_ ("BUG: OP_ILLEGAL"));
          return 1;
        }

      // not a match
      ret++;
    }
  return (ret);
}

/* This is the guts of the machine-dependent assembler.  STR points to a
   machine dependent instruction.  This function is supposed to emit
   the frags/bytes it assembles to.  */

void
md_assemble (char *str)
{
  char op[11];
  char *t = input_line_pointer;
  char *str_orig = strdup (str);
  str = skip_space (extract_word (str, op, sizeof (op)));
  // stm8_operand_t spec[3];
  expressionS exps[3];
  // memset(spec, 0, sizeof(stm8_operand_t) * 3);
  memset (exps, 0, sizeof (expressionS) * 3);

  int count = read_args (str, exps);
  struct stm8_opcodes_s *opcode
      = (struct stm8_opcodes_s *)str_hash_find (stm8_hash, op);

  if (opcode == NULL)
    {
      as_bad (_ ("unknown opcode `%s'"), op);
      return;
    }

  int i;
  for (i = 0; opcode[i].name != NULL; i++)
    {
      if (!strcmp (op, opcode[i].name))
        if (!cmpspec (opcode[i].constraints, exps, count))
          {
            int insn_size = stm8_compute_insn_size (&opcode[i]);
            char *frag = frag_more (insn_size);
            int opcode_length = stm8_opcode_size (opcode[i].bin_opcode);
            bfd_put_bits (opcode[i].bin_opcode, frag, opcode_length * 8, true);
            frag += opcode_length;

            /* mov insn operands are reversed */
            if ((opcode[i].bin_opcode == 0x35)
                || (opcode[i].bin_opcode == 0x45)
                || (opcode[i].bin_opcode == 0x55))
              {
                count = -count;
              }
            stm8_bfd_out (opcode[i], exps, count, frag);
            break;
          }
    }
  if (!opcode[i].name)
    as_bad ("Invalid instruction: %s", str_orig);
  input_line_pointer = t;
  free (str_orig);
}

/* If you define this macro, it should return the position from which
   the PC relative adjustment for a PC relative fixup should be made.
   On many processors, the base of a PC relative instruction is the
   next instruction, so this macro would return the length of an
   instruction, plus the address of the PC relative fixup.  The latter
   can be calculated as fixp->fx_where +
   fixp->fx_frag->fr_address.  */

long
md_pcrel_from_section (fixS *fixp, segT sec)
{
  if (fixp->fx_addsy != (symbolS *)NULL
      && (!S_IS_DEFINED (fixp->fx_addsy)
          || (S_GET_SEGMENT (fixp->fx_addsy) != sec)))
    return 0;

  return fixp->fx_size + fixp->fx_where + fixp->fx_frag->fr_address;
}

int
stm8_need_index_operator ()
{
  return 1;
}

operatorT
stm8_operator (const char *name ATTRIBUTE_UNUSED,
               unsigned int operands ATTRIBUTE_UNUSED,
               char *pc ATTRIBUTE_UNUSED)
{
  return O_absent;
}

void
stm8_check_label (symbolS *labelsym ATTRIBUTE_UNUSED)
{
}

extern void
stm8_symbol_new_hook (symbolS *labelsym)
{
  char *name;
  char *p;
  int value;

  name = (char *)S_GET_NAME (labelsym);
  if ((p = strstr (name, ".b")))
    {
      if (*(p + 2) == 0)
        {
          *p = 0;
          S_SET_NAME (labelsym, name);
          value = 1;
          symbol_set_tc (labelsym, &value);
        }
    }
}
