#ifndef __PExpr_H
#define __PExpr_H
/*
 * Copyright (c) 1998-2000 Stephen Williams <steve@icarus.com>
 *
 *    This source code is free software; you can redistribute it
 *    and/or modify it in source code form under the terms of the GNU
 *    General Public License as published by the Free Software
 *    Foundation; either version 2 of the License, or (at your option)
 *    any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */
#if !defined(WINNT) && !defined(macintosh)
#ident "$Id: PExpr.h,v 1.51 2001/11/07 04:26:46 steve Exp $"
#endif

# include  <string>
# include  "netlist.h"
# include  "verinum.h"
# include  "verireal.h"
# include  "LineInfo.h"

class Design;
class Module;
class NetNet;
class NetExpr;
class NetScope;

/*
 * The PExpr class hierarchy supports the description of
 * expressions. The parser can generate expression objects from the
 * source, possibly reducing things that it knows how to reduce.
 *
 * The elaborate_net method is used by structural elaboration to build
 * up a netlist interpretation of the expression.
 */

class PExpr : public LineInfo {

    public:
      PExpr();
      virtual ~PExpr();

      virtual void dump(ostream&) const;

	// Procedural elaboration of the expression.
      virtual NetExpr*elaborate_expr(Design*des, NetScope*scope) const;

	// Elaborate expressions that are the r-value of parameter
	// assignments. This elaboration follows the restrictions of
	// constant expressions and supports later overriding and
	// evaluation of parameters.
      virtual NetExpr*elaborate_pexpr(Design*des, NetScope*sc) const;

	// This method elaborate the expression as gates, for use in a
	// continuous assign or other wholly structural context.
      virtual NetNet* elaborate_net(Design*des, const string&path,
				    unsigned lwidth,
				    unsigned long rise,
				    unsigned long fall,
				    unsigned long decay,
				    Link::strength_t drive0 =Link::STRONG,
				    Link::strength_t drive1 =Link::STRONG)
	    const;

	// This method elaborates the expression as NetNet objects. It
	// only allows regs suitable for procedural continuous assignments.
      virtual NetNet* elaborate_anet(Design*des, NetScope*scope) const;

	// This method elaborates the expression as gates, but
	// restricted for use as l-values of continuous assignments.
      virtual NetNet* elaborate_lnet(Design*des, NetScope*scope) const;

	// Expressions that can be in the l-value of procedural
	// assignments can be elaborated with this method.
      virtual NetAssign_* elaborate_lval(Design*des, NetScope*scope) const;

	// This attempts to evaluate a constant expression, and return
	// a verinum as a result. If the expression cannot be
	// evaluated, return 0.
      virtual verinum* eval_const(const Design*des, const NetScope*sc) const;

	// This attempts to evaluate a constant expression as a
	// decimal floating point. This is used when calculating delay
	// constants.
      virtual verireal* eval_rconst(const Design*, const NetScope*) const;

	// This method returns true if that expression is the same as
	// this expression. This method is used for comparing
	// expressions that must be structurally "identical".
      virtual bool is_the_same(const PExpr*that) const;

	// Return true if this expression is a valid constant
	// expression. the Module pointer is needed to find parameter
	// identifiers and any other module specific interpretations
	// of expresions.
      virtual bool is_constant(Module*) const;

    private: // not implemented
      PExpr(const PExpr&);
      PExpr& operator= (const PExpr&);
};

ostream& operator << (ostream&, const PExpr&);

class PEConcat : public PExpr {

    public:
      PEConcat(const svector<PExpr*>&p, PExpr*r =0);
      ~PEConcat();

      virtual void dump(ostream&) const;

	// Concatenated Regs can be on the left of procedural
	// continuous assignments.
      virtual NetNet* elaborate_anet(Design*des, NetScope*scope) const;

      virtual NetNet* elaborate_lnet(Design*des, NetScope*scope) const;
      virtual NetNet* elaborate_net(Design*des, const string&path,
				    unsigned width,
				    unsigned long rise,
				    unsigned long fall,
				    unsigned long decay,
				    Link::strength_t drive0,
				    Link::strength_t drive1) const;
      virtual NetExpr*elaborate_expr(Design*des, NetScope*) const;
      virtual NetEConcat*elaborate_pexpr(Design*des, NetScope*) const;
      virtual NetAssign_* elaborate_lval(Design*des, NetScope*scope) const;
      virtual bool is_constant(Module*) const;

    private:
      svector<PExpr*>parms_;
      PExpr*repeat_;
};

/*
 * Event expressions are expressions that can be combined with the
 * event "or" operator. These include "posedge foo" and similar, and
 * also include named events. "edge" events are associated with an
 * expression, whereas named events simply have a name, which
 * represents an event variable.
 */
class PEEvent : public PExpr {

    public:
      enum edge_t {ANYEDGE, POSEDGE, NEGEDGE, POSITIVE};

	// Use this constructor to create events based on edges or levels.
      PEEvent(edge_t t, PExpr*e);

      ~PEEvent();

      edge_t type() const;
      PExpr* expr() const;
 
      virtual void dump(ostream&) const;

    private:
      edge_t type_;
      PExpr *expr_;
};

/*
 * This holds a floating point constant in the source.
 */
class PEFNumber : public PExpr {

    public:
      explicit PEFNumber(verireal*vp);
      ~PEFNumber();

      const verireal& value() const;

	/* The eval_const method as applied to a floating point number
	   gets the *integer* value of the number. This accounts for
	   any rounding that is needed to get the value. */
      virtual verinum* eval_const(const Design*des, const NetScope*sc) const;

	/* This method returns the full floating point value. */
      virtual verireal* eval_rconst(const Design*, const NetScope*) const;

	/* A PEFNumber is a constant, so this returns true. */
      virtual bool is_constant(Module*) const;

      virtual NetExpr*elaborate_expr(Design*des, NetScope*) const;
      virtual NetExpr*elaborate_pexpr(Design*des, NetScope*sc) const;

      virtual void dump(ostream&) const;

    private:
      verireal*value_;
};

class PEIdent : public PExpr {

    public:
      explicit PEIdent(const string&s);
      ~PEIdent();

      virtual void dump(ostream&) const;

	// Regs can be on the left of procedural continuous assignments
      virtual NetNet* elaborate_anet(Design*des, NetScope*scope) const;

	// Identifiers are allowed (with restrictions) is assign l-values.
      virtual NetNet* elaborate_lnet(Design*des, NetScope*scope) const;

	// Identifiers are also allowed as procedural assignment l-values.
      virtual NetAssign_* elaborate_lval(Design*des, NetScope*scope) const;

	// Structural r-values are OK.
      virtual NetNet* elaborate_net(Design*des, const string&path,
				    unsigned lwidth,
				    unsigned long rise,
				    unsigned long fall,
				    unsigned long decay,
				    Link::strength_t drive0,
				    Link::strength_t drive1) const;

      virtual NetExpr*elaborate_expr(Design*des, NetScope*) const;
      virtual NetExpr*elaborate_pexpr(Design*des, NetScope*sc) const;

	// Elaborate the PEIdent as a port to a module. This method
	// only applies to Ident expressions.
      NetNet* elaborate_port(Design*des, NetScope*sc) const;

      virtual bool is_constant(Module*) const;
      verinum* eval_const(const Design*des, const NetScope*sc) const;
      verireal*eval_rconst(const Design*des, const NetScope*sc) const;

      string name() const;

    private:
      string text_;

    public:
	// Use these to support bit- and part-select operators.
      PExpr*msb_;
      PExpr*lsb_;

	// If this is a reference to a memory, this is the index
	// expression.
      PExpr*idx_;

      NetNet* elaborate_net_ram_(Design*des, const string&path,
				 NetMemory*mem, unsigned lwidth,
				 unsigned long rise,
				 unsigned long fall,
				 unsigned long decay) const;
};

class PENumber : public PExpr {

    public:
      explicit PENumber(verinum*vp);
      ~PENumber();

      const verinum& value() const;

      virtual void dump(ostream&) const;
      virtual NetNet* elaborate_net(Design*des, const string&path,
				    unsigned lwidth,
				    unsigned long rise,
				    unsigned long fall,
				    unsigned long decay,
				    Link::strength_t drive0,
				    Link::strength_t drive1) const;
      virtual NetEConst*elaborate_expr(Design*des, NetScope*) const;
      virtual NetExpr*elaborate_pexpr(Design*des, NetScope*sc) const;
      virtual verinum* eval_const(const Design*des, const NetScope*sc) const;
      virtual verireal*eval_rconst(const Design*, const NetScope*) const;

      virtual bool is_the_same(const PExpr*that) const;
      virtual bool is_constant(Module*) const;

    private:
      verinum*const value_;
};

class PEString : public PExpr {

    public:
      explicit PEString(const string&s);
      ~PEString();

      string value() const;
      virtual void dump(ostream&) const;
      virtual NetEConst*elaborate_expr(Design*des, NetScope*) const;
      virtual NetEConst*elaborate_pexpr(Design*des, NetScope*sc) const;

      virtual bool is_constant(Module*) const;

    private:
      const string text_;
};

class PEUnary : public PExpr {

    public:
      explicit PEUnary(char op, PExpr*ex);
      ~PEUnary();

      virtual void dump(ostream&out) const;
      virtual NetNet* elaborate_net(Design*des, const string&path,
				    unsigned width,
				    unsigned long rise,
				    unsigned long fall,
				    unsigned long decay,
				    Link::strength_t drive0,
				    Link::strength_t drive1) const;
      virtual NetEUnary*elaborate_expr(Design*des, NetScope*) const;
      virtual NetExpr*elaborate_pexpr(Design*des, NetScope*sc) const;
      virtual verinum* eval_const(const Design*des, const NetScope*sc) const;

      virtual bool is_constant(Module*) const;

    private:
      char op_;
      PExpr*expr_;
};

class PEBinary : public PExpr {

    public:
      explicit PEBinary(char op, PExpr*l, PExpr*r);
      ~PEBinary();

      virtual bool is_constant(Module*) const;

      virtual void dump(ostream&out) const;
      virtual NetNet* elaborate_net(Design*des, const string&path,
				    unsigned width,
				    unsigned long rise,
				    unsigned long fall,
				    unsigned long decay,
				    Link::strength_t drive0,
				    Link::strength_t drive1) const;
      virtual NetEBinary*elaborate_expr(Design*des, NetScope*) const;
      virtual NetExpr*elaborate_pexpr(Design*des, NetScope*sc) const;
      virtual verinum* eval_const(const Design*des, const NetScope*sc) const;
      virtual verireal*eval_rconst(const Design*des, const NetScope*sc) const;

    private:
      char op_;
      PExpr*left_;
      PExpr*right_;

      NetEBinary*elaborate_expr_base_(Design*, NetExpr*lp, NetExpr*rp) const;

      NetNet* elaborate_net_add_(Design*des, const string&path,
				 unsigned lwidth,
				 unsigned long rise,
				 unsigned long fall,
				 unsigned long decay) const;
      NetNet* elaborate_net_bit_(Design*des, const string&path,
				 unsigned lwidth,
				 unsigned long rise,
				 unsigned long fall,
				 unsigned long decay) const;
      NetNet* elaborate_net_cmp_(Design*des, const string&path,
				 unsigned lwidth,
				 unsigned long rise,
				 unsigned long fall,
				 unsigned long decay) const;
      NetNet* elaborate_net_div_(Design*des, const string&path,
				 unsigned lwidth,
				 unsigned long rise,
				 unsigned long fall,
				 unsigned long decay) const;
      NetNet* elaborate_net_mod_(Design*des, const string&path,
				 unsigned lwidth,
				 unsigned long rise,
				 unsigned long fall,
				 unsigned long decay) const;
      NetNet* elaborate_net_log_(Design*des, const string&path,
				 unsigned lwidth,
				 unsigned long rise,
				 unsigned long fall,
				 unsigned long decay) const;
      NetNet* elaborate_net_mul_(Design*des, const string&path,
				 unsigned lwidth,
				 unsigned long rise,
				 unsigned long fall,
				 unsigned long decay) const;
      NetNet* elaborate_net_shift_(Design*des, const string&path,
				   unsigned lwidth,
				   unsigned long rise,
				   unsigned long fall,
				   unsigned long decay) const;
};

/*
 * This class supports the ternary (?:) operator. The operator takes
 * three expressions, the test, the true result and the false result.
 */
class PETernary : public PExpr {

    public:
      explicit PETernary(PExpr*e, PExpr*t, PExpr*f);
      ~PETernary();

      virtual bool is_constant(Module*) const;

      virtual void dump(ostream&out) const;
      virtual NetNet* elaborate_net(Design*des, const string&path,
				    unsigned width,
				    unsigned long rise,
				    unsigned long fall,
				    unsigned long decay,
				    Link::strength_t drive0,
				    Link::strength_t drive1) const;
      virtual NetETernary*elaborate_expr(Design*des, NetScope*) const;
      virtual NetETernary*elaborate_pexpr(Design*des, NetScope*sc) const;
      virtual verinum* eval_const(const Design*des, const NetScope*sc) const;

    private:
      PExpr*expr_;
      PExpr*tru_;
      PExpr*fal_;
};

/*
 * This class represents a parsed call to a function, including calls
 * to system functions.
 */
class PECallFunction : public PExpr {
    public:
      explicit PECallFunction(const char*n, const svector<PExpr *> &parms);
      explicit PECallFunction(const char*n);
      ~PECallFunction();

      virtual void dump(ostream &) const;
      virtual NetExpr*elaborate_expr(Design*des, NetScope*scope) const;

    private:
      string name_;
      svector<PExpr *> parms_;

      NetExpr* elaborate_sfunc_(Design*des, NetScope*scope) const;
};

/*
 * $Log: PExpr.h,v $
 * Revision 1.51  2001/11/07 04:26:46  steve
 *  elaborate_lnet uses scope instead of string path.
 *
 * Revision 1.50  2001/11/07 04:01:59  steve
 *  eval_const uses scope instead of a string path.
 *
 * Revision 1.49  2001/11/06 06:11:55  steve
 *  Support more real arithmetic in delay constants.
 *
 * Revision 1.48  2001/01/14 23:04:55  steve
 *  Generalize the evaluation of floating point delays, and
 *  get it working with delay assignment statements.
 *
 *  Allow parameters to be referenced by hierarchical name.
 *
 * Revision 1.47  2000/12/16 19:03:30  steve
 *  Evaluate <= and ?: in parameter expressions (PR#81)
 *
 * Revision 1.46  2000/12/10 22:01:35  steve
 *  Support decimal constants in behavioral delays.
 *
 * Revision 1.45  2000/12/06 06:31:09  steve
 *  Check lvalue of procedural continuous assign (PR#29)
 *
 * Revision 1.44  2000/09/17 21:26:15  steve
 *  Add support for modulus (Eric Aardoom)
 */
#endif
