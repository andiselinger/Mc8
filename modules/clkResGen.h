#include "systemc.h"

SC_MODULE(clkResGen)
{

public:
  sc_out<bool> clk;
  sc_out<bool> tclk;
  sc_out<bool> res;

  //bool int_clksig;

  SC_CTOR(clkResGen)
  {

    SC_THREAD(clkGen);
    SC_THREAD(resetGen);
    SC_THREAD(tclkGen);

  }

private:

  void
  clkGen ()
  {
    while (true)
    {
      clk.write (false);
      wait (1, SC_MS);
      clk.write (true);
      wait (1, SC_MS);
    };
  }

  void
  resetGen ()
  {
    res.write (true);
    wait (1, SC_NS);
    res.write (false);
    wait (2.5, SC_MS);
    res.write (true);
    wait (100, SC_MS);

  }
  void
  tclkGen ()
  {
    while (true)
    {
      tclk.write (false);
      wait (1, SC_US);
      tclk.write (true);
      wait (1, SC_US);
    }
  }

};

