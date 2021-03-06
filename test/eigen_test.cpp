#include "common.hpp"
#include "../eigen/eigen_iterative_solver.hpp"
#include "../eigen/ArpackSupport"
#include <chrono>
#include <triqs/mc_tools/random_generator.hpp>


using namespace fk;

int main(int argc, char* argv[])
{

    typedef Eigen::SparseMatrix<double> sparse_m;
    typedef Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> dense_m;
    triqs::mc_tools::random_generator RNG("mt19937", 23432);

    size_t size = 10;
    size_t nonzero_elems = size*3;
    //EMatrixType<double> a(10,10);
    //a.setRandom();
    sparse_m a(size,size); 
    a.reserve(2*nonzero_elems);
    for (size_t l=0; l<nonzero_elems; l++) { 
        size_t i = RNG(size), j = i+RNG(size-i); 
        while(a.coeffRef(i,j)!=0) {i = RNG(size); j = RNG(size);};
        a.coeffRef(i,j) = RNG(2.0)-1.;
        a.coeffRef(j,i) = a.coeffRef(i,j);
        };
    INFO(a);

    dense_m b(a);
    INFO(b);
    Eigen::SelfAdjointEigenSolver<dense_m> s(b);
    INFO(s.eigenvalues());
    //a = a.selfadjointView<Eigen::Upper>();
    INFO("");
    Eigen::ArpackGeneralizedSelfAdjointEigenSolver<sparse_m> solver(a,2,"SA",Eigen::EigenvaluesOnly,1e-5);
    INFO(solver.eigenvalues().reverse());
    INFO("----------");

    Eigen::ArpackGeneralizedSelfAdjointEigenSolver<sparse_m> solver2(a,size-2,"SA",Eigen::EigenvaluesOnly);
    INFO(solver2.eigenvalues().reverse());

    auto t_in = s.eigenvalues();
    t_in(0) = 1.;
    //t_in(1) = -1.29;
    //Eigen::IterativeInverseEigenSolver<sparse_m> solver_inv_1(a,2,t_in);
    //Eigen::IterativeInverseEigenSolver<dense_m, Eigen::LDLT<dense_m>> solver_inv_1(b,2,t_in, Eigen::EigenvaluesOnly, 1e-8, 1000);
    dense_m ev_in(b.rows(),2);
    ev_in.col(0) = s.eigenvectors().col(0);
    ev_in.col(1) = s.eigenvectors().col(1);
    ev_in.col(1).setRandom();
    Eigen::IterativeInverseEigenSolver<dense_m, Eigen::LDLT<dense_m>> solver_inv_1(b,2,t_in.head(2), ev_in, Eigen::EigenvaluesOnly, 1e-8, 1000);
    return EXIT_SUCCESS;
}
