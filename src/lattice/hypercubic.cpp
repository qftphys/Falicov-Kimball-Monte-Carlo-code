#include "fk_mc/lattice/hypercubic.hpp"

namespace fk { 

template <size_t D>
hypercubic_lattice<D>::hypercubic_lattice(size_t lattice_size):
    lattice_base(sparse_m(boost::math::pow<D>(lattice_size), boost::math::pow<D>(lattice_size))),
    ft_pi_array_(m_size_)
{
    hopping_m_.reserve(Eigen::ArrayXi::Constant(m_size_,D*2));
    dims.fill(lattice_size);
    for (size_t i=0; i<m_size_; i++) { 
        auto pos = index_to_pos(i); 
        int v = 1;
        for (int p : pos) v*=((p%2)*2-1);
        ft_pi_array_[i]=v;
        }; 
};

template <size_t D>
std::array<int, D> hypercubic_lattice<D>::index_to_pos(size_t index) const
{
    std::array<int, D> out;
    for (int i=D-1; i>=0; i--) {
        out[i]=index%dims[i];
        index/=dims[i];
    };
    return out;
}

template <size_t D>
size_t hypercubic_lattice<D>::pos_to_index(std::array<int, D> pos) const
{
    size_t out=0;
    size_t mult = 1;
    for (int i=D-1; i>=0; i--) {
        out+=pos[i]*mult;
        mult*=dims[i];
    };
    return out;
}

template <size_t D>
std::array<std::array<int, D>, 2*D> hypercubic_lattice<D>::neighbor_pos(std::array<int, D> pos) const {
    std::array<std::array<int, D>, 2*D> result;

    for (int i = 0; i < D; ++i) {
        result[2*i].fill(0);
        result[2*i+1].fill(0);
        result[2*i][i] = pos[i] == 0 ? dims[i] - 1 : pos[i] - 1;
        result[2*i+1][i] = pos[i] == dims[i] - 1 ? 0 : pos[i] + 1;
    }

    return result;
}

template <size_t D>
std::vector<size_t> hypercubic_lattice<D>::neighbor_index(size_t index) const {
    std::vector<size_t> result(2*D);
    auto positions = neighbor_pos(index_to_pos(index));
    std::transform(std::begin(positions), std::end(positions),
                   std::begin(result),
                   [this](const std::array<int, D> &p) { return pos_to_index(p); });
    return result;
}

template <size_t D>
int hypercubic_lattice<D>::FFT_pi(const Eigen::ArrayXi& in) const
{
    return (in*ft_pi_array_).sum();
}


template <size_t D>
typename hypercubic_lattice<D>::BZPoint hypercubic_lattice<D>::get_bzpoint(std::array<double, D> in) const
{
    return BZPoint(in, *this);
}

template <size_t D>
typename hypercubic_lattice<D>::BZPoint hypercubic_lattice<D>::get_bzpoint(size_t in) const
{
    return BZPoint(in, *this);
}
    
template <size_t D>
std::vector<typename hypercubic_lattice<D>::BZPoint> hypercubic_lattice<D>::get_all_bzpoints() const
{
    int npts=1; for (auto x:dims) npts*=x;
    std::vector<BZPoint> out;
    out.reserve(npts);
    for (int i=0; i<npts; i++) out.push_back(this->get_bzpoint(i));
    return out;
}

template <size_t D>
void hypercubic_lattice<D>::fill(double t)
{
    for (size_t i=0; i<m_size_; ++i) {
        auto current_pos = index_to_pos(i);
        for (size_t n=0; n<D; ++n) {
            auto pos_l(current_pos), pos_r(current_pos);
            pos_l[n]=(current_pos[n]>0?current_pos[n]-1:dims[n]-1);
            pos_r[n]=(current_pos[n]<dims[n]-1?current_pos[n]+1:0);
            hopping_m_.insert(i,pos_to_index(pos_l)) = -1.0*t;
            hopping_m_.insert(i,pos_to_index(pos_r)) = -1.0*t;
        }; 
    };
}


template struct hypercubic_lattice<1>;
template struct hypercubic_lattice<2>;
template struct hypercubic_lattice<3>;
//template struct hypercubic_lattice<4>;

} // end of namespace fk
