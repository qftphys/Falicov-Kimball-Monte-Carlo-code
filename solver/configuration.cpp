#include "configuration.hpp"

#include "../eigen/ArpackSupport"

namespace fk {

bool config_params::operator== ( const config_params& rhs) const
{
    double tol = std::numeric_limits<double>::epsilon(); 
    return (std::abs(beta - rhs.beta) < tol && std::abs(U - rhs.U) < tol && std::abs(mu_c - rhs.mu_c) < tol && std::abs(mu_f - rhs.mu_f) < tol);
}

configuration_t::configuration_t(
    const lattice_base &lattice, double beta, double U, double mu_c, double mu_f):
    lattice_(lattice),
    f_config_(lattice_.get_msize()),
    params_(config_params({beta, U, mu_c, mu_f})),
    hamilt_(lattice_.hopping_m.rows(), lattice_.hopping_m.cols())
{ 
    f_config_.setZero(); 
}

/*
void configuration_t::swap(configuration_t &rhs)
{
    f_config_.swap(rhs.f_config_); 
    cached_spectrum.swap(rhs.cached_spectrum);
    cached_weights.swap(rhs.cached_weights);
    hamilt_.swap(rhs.hamilt_);
}*/

configuration_t& configuration_t::operator=(const configuration_t& rhs) 
{
    f_config_ = rhs.f_config_; 
    hamilt_ = rhs.hamilt_;
    ed_data_ = rhs.ed_data_;
    cheb_data_ = rhs.cheb_data_;
    if (!(params_ == rhs.params_)) throw (std::logic_error("Mismatched parameters in config assignment"));
    return *this;
};

size_t configuration_t::get_nf() const
{
    return std::accumulate(f_config_.data(), f_config_.data()+lattice_.get_msize(), 0);
}

void configuration_t::randomize_f(triqs::mc_tools::random_generator &rnd, size_t nf){
    if (!nf) nf = rnd(lattice_.get_msize());
    f_config_.setZero();
    for (size_t i=0; i<nf; ++i) {  
    size_t ind = rnd(lattice_.get_msize());
    while (f_config_(ind)==1) ind = rnd(lattice_.get_msize());
    f_config_(ind) = 1; 
    };
}

const typename configuration_t::sparse_m& configuration_t::calc_hamiltonian()
{
    reset_cache();
    hamilt_.reserve(lattice_.hopping_m.nonZeros() + lattice_.get_msize());
    hamilt_ = lattice_.hopping_m;
    for (size_t i=0; i<lattice_.get_msize(); ++i) hamilt_.coeffRef(i,i)+= -params_.mu_c + params_.U*f_config_(i); // unoptimized
    return hamilt_;
}

void configuration_t::calc_chebyshev( const chebyshev::chebyshev_eval& cheb)
{

    if (int(cheb_data_.status) >= int(chebyshev_cache::logz)) return;
    sparse_m x = hamilt_; 
    size_t msize = lattice_.get_msize();
    double e_min = Eigen::ArpackGeneralizedSelfAdjointEigenSolver<sparse_m>(hamilt_,1,"SA",Eigen::EigenvaluesOnly).eigenvalues()[0];
    double e_max = Eigen::ArpackGeneralizedSelfAdjointEigenSolver<sparse_m>(hamilt_,1,"LA",Eigen::EigenvaluesOnly).eigenvalues()[0];
    double a = (e_max - e_min)/2.;
    double b = (e_max + e_min)/2.; 
    double beta = params_.beta;
    cheb_data_.e_min = e_min;
    cheb_data_.e_max = e_max;
    cheb_data_.a = a;
    cheb_data_.b = b;

    for (size_t i=0; i<msize; ++i) x.coeffRef(i,i)+= -b; // unoptimized
    x/=a;

    size_t cheb_size = cheb.cheb_size();
    cheb_data_.moments.resize(cheb_size);
    dense_m cm0 = dense_m::Identity(msize,msize);
    dense_m cm1 = x*cm0;

    auto logz_f = [a,b,beta,msize](double w){return msize*log(1. + exp(-beta*(a*w+b)));}; 

    double s=cm0.diagonal().sum()/msize * cheb.moment(logz_f, 0);
    dense_m cm_tmp = cm1;
    for (int i=1; i<cheb_size; i++) {
            if (i>1) { cm_tmp = x*2*cm1 - cm0; cm0.swap(cm1); cm1.swap(cm_tmp); }
            cheb_data_.moments[i] = cm1.diagonal().sum()/msize;
            double s_m2 = cheb.moment(logz_f, i);
            s+=2.*s_m2*cheb_data_.moments[i];
    //        std::cout << "moment [" << i << "] = " << cheb_data_.moments[i] << std::endl;
    //        std::cout << s_m2 << " | " << s << std::endl;
        }

    cheb_data_.logZ = s;
    cheb_data_.x.swap(x);
    cheb_data_.status = chebyshev_cache::logz;
}

void configuration_t::calc_ed(bool calc_evecs)
{
    if ( (ed_data_.status == ed_cache::spectrum && !calc_evecs) || (ed_data_.status == ed_cache::full && calc_evecs)) return;

    dense_m h(hamilt_);
    Eigen::SelfAdjointEigenSolver<dense_m> s(h,(calc_evecs?Eigen::ComputeEigenvectors:Eigen::EigenvaluesOnly));
    ed_data_.cached_spectrum = s.eigenvalues();
    ed_data_.status = ed_cache::spectrum;
    if (calc_evecs) {
        ed_data_.cached_evecs = s.eigenvectors();
        ed_data_.status = ed_cache::full;
        };
    //auto s2 = cached_spectrum;
    //std::sort (cached_spectrum.data(), cached_spectrum.data()+cached_spectrum.size());  
    //DEBUG((Eigen::VectorXd(cached_spectrum - s2)).squaredNorm());

    const auto& cached_spectrum = ed_data_.cached_spectrum;

    double beta = params_.beta;

    double e0 = cached_spectrum[0];
    double logw0 = beta * e0;
    double weight0 = exp(logw0);

    double logz = 0.0;
    for (size_t i=0; i<cached_spectrum.size(); ++i) { 
        double e = cached_spectrum[i]; 
        double w = exp(-beta*(e-e0)); 
        logz += std::log(weight0 + w) - logw0;
        };

    ed_data_.logZ = logz; 

}



} // end of namespace fk
