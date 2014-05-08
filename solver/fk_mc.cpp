#include "fk_mc.hpp"
#include "moves.hpp"
#include "moves_chebyshev.hpp"
#include "measures/energy.hpp"
#include "measures/spectrum.hpp"
#include "measures/spectrum_history.hpp"
#include "measures/focc_history.hpp"

#include <triqs/utility/callbacks.hpp>

namespace fk {

triqs::utility::parameters _update_def(triqs::utility::parameters p){p.update(fk_mc::solve_defaults()); return p;}

fk_mc::fk_mc(const lattice_base& l, utility::parameters p1):
    lattice(l),
    p(_update_def(p1)),
    //cheb_eval(chebyshev_eval(p["cheb_size"], p["cheb_grid_size"])),
    config(l,p["beta"],p["U"],p["mu_c"],p["mu_f"]),
    mc(p) 
{
    INFO("\tRandom seed for proc " << world.rank() << " : " << p["random_seed"]);
}

void fk_mc::solve()
{
    if (world.rank() == 0) std::cout << "Running MC..." << std::endl << std::endl;

    // Generate the configuration_t and cache the spectrum
    double beta = p["beta"];
    config.randomize_f(mc.rng(),p["Nf_start"]);
    config.calc_hamiltonian();

    std::unique_ptr<chebyshev::chebyshev_eval> cheb_ptr;

    bool cheb_move = p["cheb_moves"];
    if (cheb_move) {
        int cheb_size = int(std::log(lattice.get_msize()) * double(p["cheb_prefactor"]));
        cheb_size+=cheb_size%2;
        size_t ngrid_points = std::max(cheb_size*2,10);
        cheb_ptr.reset(new chebyshev::chebyshev_eval(cheb_size, ngrid_points));
    }
        

    if (double(p["mc_flip"])>std::numeric_limits<double>::epsilon()) { 
        if (!cheb_move) mc.add_move(move_flip(beta, config, mc.rng()), "flip", p["mc_flip"]); 
                   else mc.add_move(chebyshev::move_flip(beta, config, *cheb_ptr, mc.rng()), "flip", p["mc_flip"]); 
        };
    if (double(p["mc_add_remove"])>std::numeric_limits<double>::epsilon()) { 
        if (!cheb_move) mc.add_move(move_addremove(beta, config, mc.rng()), "add_remove", p["mc_add_remove"]);
                   else mc.add_move(chebyshev::move_addremove(beta, config, *cheb_ptr, mc.rng()), "add_remove", p["mc_add_remove"]);
        };
    if (double(p["mc_reshuffle"])>std::numeric_limits<double>::epsilon()) { 
        if (!cheb_move) mc.add_move(move_randomize(beta, config, mc.rng()),  "reshuffle", p["mc_reshuffle"]);
                   else mc.add_move(chebyshev::move_randomize(beta, config, *cheb_ptr, mc.rng()), "reshuffle", p["mc_reshuffle"]);
        };

    size_t max_bins = p["n_cycles"];
    observables.energies.reserve(max_bins);
    observables.d2energies.reserve(max_bins);
    mc.add_measure(measure_energy(beta,config,observables.energies, observables.d2energies), "energy");
    mc.add_measure(measure_spectrum(config,observables.spectrum), "spectrum");
    if (p["measure_history"]) {
        mc.add_measure(measure_spectrum_history(config,observables.spectrum_history), "spectrum_history");
        mc.add_measure(measure_focc(config,observables.focc_history), "focc_history");
        };

      // run and collect results
    mc.start(1.0, triqs::utility::clock_callback(p["max_time"]));
    mc.collect_results(world);
}

 triqs::utility::parameter_defaults fk_mc::solve_defaults() {

  triqs::utility::parameter_defaults pdef;

  pdef.required
   ("beta", double(), "Inverse temperature")
   ("U", double(1.0), "FK U")
   ("n_cycles", int(), "Number of QMC cycles")
   ("mu_c", double(0.5), "Chemical potential of c electrons")
   ("mu_f", double(0.5), "Chemical potential of f electrons")
   ;

  pdef.optional
   ("mc_flip", double(0.0), "Make flip moves")
   ("mc_add_remove", double(1.0), "Make add/remove moves")
   ("mc_reshuffle", double(0.0), "Make reshuffle moves")
   ("cheb_moves", bool(false), "Allow moves using Chebyshev sampling")
   ("cheb_prefactor", double(2.2), "Prefactor for number of Chebyshev polynomials = #ln(Volume)")
   ("measure_history", bool(true), "Measure the history")
   ("random_name", std::string(""), "Name of random number generator")
   ("Nf_start", size_t(5), "Starting number of f-electrons")
   ("length_cycle", int(50), "Length of a single QMC cycle")
   ("n_warmup_cycles", int(5000), "Number of cycles for thermalization")
   ("random_seed", int(34788), "Seed for random number generator")
   ("max_time",int(600000), "Maximum running time")
   ;

  return pdef;
 }

} // end of namespace FK
