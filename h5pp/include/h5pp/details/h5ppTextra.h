#pragma once
#include <Eigen/Core>
#include <Eigen/Sparse>
#include <iostream>
#include <iterator>
#include <numeric>
#include <unsupported/Eigen/CXX11/Tensor>
#if !defined(_MSC_VER)
#define CONSTEXPR constexpr
#else
#define CONSTEXPR
#endif

namespace h5pp {
    /*! \brief **Textra** stands for "Tensor Extra". Provides extra functionality to Eigen::Tensor.*/

    /*!
     *  \namespace Textra
     *  This namespace makes shorthand typedef's to Eigen's unsupported Tensor module, and provides handy functions
     *  to interface between `Eigen::Tensor` and `Eigen::Matrix` objects.
     *  The contents of this namespace is co clear it is self-documenting ;)
     */
    namespace Textra {
        using cdouble = std::complex<double>;
        // The default type for an index varies wildly between compilers/archs, so we query here
        using idxType                                    = typename Eigen::Tensor<double, 1>::Index;
        template<typename Scalar> using MatrixType       = Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>;
        template<typename Scalar> using VectorType       = Eigen::Matrix<Scalar, Eigen::Dynamic, 1>;
        template<typename Scalar> using SparseMatrixType = Eigen::SparseMatrix<Scalar>;
        template<idxType rank> using array               = Eigen::array<idxType, rank>;
        template<idxType rank> using DSizes              = Eigen::DSizes<idxType, rank>;
        using array8                                     = array<8>;
        using array7                                     = array<7>;
        using array6                                     = array<6>;
        using array5                                     = array<5>;
        using array4                                     = array<4>;
        using array3                                     = array<3>;
        using array2                                     = array<2>;
        using array1                                     = array<1>;

        // Shorthand for the list of index pairs.
        template<typename T, auto length> using idxlistpair = Eigen::array<Eigen::IndexPair<T>, length>;

        // Handy functions to copy lists of dimensions
        template<typename T, auto rank, typename Container> void copy_dims(Eigen::DSizes<T, rank> dsizes, const Container &container) {
            if constexpr(std::is_array_v<Container>) {
                std::copy_n(std::begin(container), rank, dsizes.begin());
            } else {
                if(container.size() != rank) throw std::runtime_error("copy_dims: Wrong container size, can't copy dimensions.");
                std::copy(std::begin(container), std::end(container), dsizes.begin());
            }
        }
        template<auto rank, typename Container>[[nodiscard]] CONSTEXPR Eigen::DSizes<idxType, rank> copy_dims(const Container &container) {
            Eigen::DSizes<idxType, rank> dsizes;
            if constexpr(std::is_array<Container>::value) {
                std::copy_n(std::begin(container), rank, dsizes.begin());
            } else {
                if(container.size() != rank) throw std::runtime_error("copy_dims: Wrong container size, can't copy dimensions.");
                std::copy(std::begin(container), std::end(container), dsizes.begin());
            }
            return dsizes;
        }

        template<auto rank, typename Container>[[nodiscard]] CONSTEXPR Eigen::DSizes<idxType, rank> copy_dims(const Container *container) {
            Eigen::DSizes<idxType, rank> dsizes;
            std::copy_n(container, rank, dsizes.begin());
            return dsizes;
        }

        CONSTEXPR idxlistpair<idxType, 0> idx() {
            Eigen::array<Eigen::IndexPair<idxType>, 0> empty_index_list = {};
            return empty_index_list;
        }

        template<typename T, size_t N> CONSTEXPR idxlistpair<idxType, N> idx(const T (&list1)[N], const T (&list2)[N]) {
            // Use numpy-style indexing for contraction. Each list contains a list of indices to be contracted for the respective
            // tensors. This function zips them together into pairs as used in Eigen::Tensor module. This does not sort the indices in decreasing order.
            static_assert(std::is_integral_v<T>);
            Eigen::array<Eigen::IndexPair<idxType>, N> pairlistOut;
            for(size_t i = 0; i < N; i++) { pairlistOut[i] = Eigen::IndexPair<idxType>{list1[i], list2[i]}; }
            return pairlistOut;
        }

        template<typename T> struct idx_dim_pair {
            T idxA;
            T idxB;
            T dimB;
        };

        template<size_t NB, size_t N> CONSTEXPR idxlistpair<idxType, N> sortIdx(const array<NB> &dimensions, const idxType (&idx_ctrct_A)[N], const idxType (&idx_ctrct_B)[N]) {
            // When doing contractions, some indices may be larger than others. For performance, you want to
            // contract the largest indices first. This will return a sorted index list in decreasing order.
            Eigen::array<idx_dim_pair<idxType>, N> idx_dim_pair_list;
            for(size_t i = 0; i < N; i++) { idx_dim_pair_list[i] = {idx_ctrct_A[i], idx_ctrct_B[i], dimensions[idx_ctrct_B[i]]}; }
            std::sort(idx_dim_pair_list.begin(), idx_dim_pair_list.end(), [](const auto &i, const auto &j) { return i.dimB > j.dimB; });
            idxlistpair<idxType, N> pairlistOut;
            for(size_t i = 0; i < N; i++) { pairlistOut[i] = Eigen::IndexPair<idxType>{idx_dim_pair_list[i].idxA, idx_dim_pair_list[i].idxB}; }
            return pairlistOut;
        }

        //
        //    //***************************************//
        //    //Different views for rank 1 and 2 tensors//
        //    //***************************************//
        //

        template<typename Scalar> constexpr Eigen::Tensor<Scalar, 1> extractDiagonal(const Eigen::Tensor<Scalar, 2> &tensor) {
            auto rows = tensor.dimension(0);
            auto cols = tensor.dimension(1);
            assert(tensor.dimension(0) == tensor.dimension(1) and "extractDiagonal expects a square tensor");

            Eigen::Tensor<Scalar, 1> diagonals(rows);
            for(auto i = 0; i < rows; i++) { diagonals(i) = tensor(i, i); }
            return diagonals;
        }

        template<typename Scalar> constexpr auto asDiagonal(const Eigen::Tensor<Scalar, 1> &tensor) {
            return tensor.inflate(array1{tensor.size() + 1}).reshape(array2{tensor.size(), tensor.size()});
        }

        template<typename Scalar> constexpr auto asDiagonalSquared(const Eigen::Tensor<Scalar, 1> &tensor) {
            return tensor.square().inflate(array1{tensor.size() + 1}).reshape(array2{tensor.size(), tensor.size()});
        }

        template<typename Scalar> constexpr auto asDiagonalInversed(const Eigen::Tensor<Scalar, 1> &tensor) {
            return tensor.inverse().inflate(array1{tensor.size() + 1}).reshape(array2{tensor.size(), tensor.size()});
        }

        template<typename Scalar> constexpr auto asDiagonalInversed(const Eigen::Tensor<Scalar, 2> &tensor) {
            assert(tensor.dimension(0) == tensor.dimension(1) and "Textra::asDiagonalInversed expects a square tensor");
            Eigen::Tensor<Scalar, 2> inversed = asDiagonalInversed(extractDiagonal(tensor));
            std::cout << "inversed:\n" << inversed << std::endl;
            return inversed;
        }

        template<typename Scalar> constexpr auto asNormalized(const Eigen::Tensor<Scalar, 1> &tensor) {
            Eigen::Map<const VectorType<Scalar>> map(tensor.data(), tensor.size());
            return Eigen::TensorMap<Eigen::Tensor<const Scalar, 1>>(map.normalized().eval().data(), array1{map.size()});
        }

        //    //****************************//
        //    //Matrix to tensor conversions//
        //    //****************************//

        // Detects if Derived is a plain object, like "MatrixXd" or similar.
        // std::decay removes pointer or ref qualifiers if present
        template<typename Derived> using is_plainObject = std::is_base_of<Eigen::PlainObjectBase<std::decay_t<Derived>>, std::decay_t<Derived>>;

        template<typename Derived, auto rank>
        constexpr Eigen::Tensor<typename Derived::Scalar, rank> Matrix_to_Tensor(const Eigen::EigenBase<Derived> &matrix, const array<rank> &dims) {
            if constexpr(is_plainObject<Derived>::value) {
                // Return map from raw input.
                return Eigen::TensorMap<const Eigen::Tensor<const typename Derived::Scalar, rank>>(matrix.derived().eval().data(), dims);
            } else {
                // Create a temporary
                MatrixType<typename Derived::Scalar> matref = matrix;
                return Eigen::TensorMap<Eigen::Tensor<typename Derived::Scalar, rank>>(matref.data(), dims);
            }
        }

        // Helpful overload
        template<typename Derived, typename... Dims>
        constexpr Eigen::Tensor<typename Derived::Scalar, sizeof...(Dims)> Matrix_to_Tensor(const Eigen::EigenBase<Derived> &matrix, const Dims... dims) {
            return Matrix_to_Tensor(matrix, array<sizeof...(Dims)>{dims...});
        }
        // Helpful overload
        template<typename Derived, auto rank>
        constexpr Eigen::Tensor<typename Derived::Scalar, rank> Matrix_to_Tensor(const Eigen::EigenBase<Derived> &matrix, const DSizes<rank> &dims) {
            array<rank> dim_array = dims;
            std::copy(std::begin(dims), std::end(dims), std::begin(dim_array));
            return Matrix_to_Tensor(matrix, dim_array);
        }

        template<typename Derived> constexpr auto Matrix_to_Tensor1(const Eigen::EigenBase<Derived> &matrix) { return Matrix_to_Tensor(matrix, matrix.size()); }
        template<typename Derived> constexpr auto Matrix_to_Tensor2(const Eigen::EigenBase<Derived> &matrix) { return Matrix_to_Tensor(matrix, matrix.rows(), matrix.cols()); }

        //
        //    //****************************//
        //    //Tensor to matrix conversions//
        //    //****************************//
        //

        template<typename Scalar> constexpr MatrixType<Scalar> Tensor2_to_Matrix(const Eigen::Tensor<Scalar, 2> &tensor) {
            return Eigen::Map<const MatrixType<Scalar>>(tensor.data(), tensor.dimension(0), tensor.dimension(1));
        }

        template<typename Scalar> constexpr MatrixType<Scalar> Tensor1_to_Vector(const Eigen::Tensor<Scalar, 1> &tensor) {
            return Eigen::Map<const VectorType<Scalar>>(tensor.data(), tensor.size());
        }

        template<typename Scalar, auto rank, typename sizeType>
        constexpr MatrixType<Scalar> Tensor_to_Matrix(const Eigen::Tensor<Scalar, rank> &tensor, const sizeType rows, const sizeType cols) {
            return Eigen::Map<const MatrixType<Scalar>>(tensor.data(), rows, cols);
        }

        template<typename Scalar> constexpr SparseMatrixType<Scalar> Tensor2_to_SparseMatrix(const Eigen::Tensor<Scalar, 2> &tensor, double prune_threshold = 1e-15) {
            return Eigen::Map<const MatrixType<Scalar>>(tensor.data(), tensor.dimension(0), tensor.dimension(1)).sparseView().pruned(prune_threshold);
        }

        //************************//
        // change storage layout //
        //************************//
        template<typename Scalar, auto rank> Eigen::Tensor<Scalar, rank, Eigen::RowMajor> to_RowMajor(const Eigen::Tensor<Scalar, rank, Eigen::ColMajor> &tensor) {
            array<rank> neworder;
            std::iota(std::begin(neworder), std::end(neworder), 0);
            std::reverse(neworder.data(), neworder.data() + neworder.size());
            return tensor.swap_layout().shuffle(neworder);
        }
        template<typename Scalar, auto rank> Eigen::Tensor<Scalar, rank, Eigen::RowMajor> to_RowMajor(const Eigen::Tensor<Scalar, rank, Eigen::RowMajor> &tensor) { return tensor; }

        template<typename Scalar, auto rank> Eigen::Tensor<Scalar, rank, Eigen::ColMajor> to_ColMajor(const Eigen::Tensor<Scalar, rank, Eigen::RowMajor> &tensor) {
            array<rank> neworder;
            std::iota(std::begin(neworder), std::end(neworder), 0);
            std::reverse(neworder.data(), neworder.data() + neworder.size());
            return tensor.swap_layout().shuffle(neworder);
        }

        template<typename Scalar, auto rank> Eigen::Tensor<Scalar, rank, Eigen::ColMajor> to_ColMajor(const Eigen::Tensor<Scalar, rank, Eigen::ColMajor> &tensor) { return tensor; }

        template<typename Derived> Eigen::Matrix<typename Derived::Scalar, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> to_RowMajor(const Eigen::MatrixBase<Derived> &matrix) {
            if(matrix.IsRowMajor) { return matrix; }
            Eigen::Matrix<typename Derived::Scalar, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> matrowmajor = matrix;
            return matrowmajor;
        }

        template<typename Derived> Eigen::Matrix<typename Derived::Scalar, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> to_ColMajor(const Eigen::MatrixBase<Derived> &matrix) {
            if(not matrix.IsRowMajor) { return matrix; }
            Eigen::Matrix<typename Derived::Scalar, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> matrowmajor = matrix;
            return matrowmajor;
        }

    }

    //******************************************************//
    // std::cout overloads for dimension() and array objects //
    //******************************************************//

    template<typename T, int L> std::ostream &operator<<(std::ostream &out, const Eigen::DSizes<T, L> &v) {
        if(!v.empty()) {
            out << "[ ";
            std::copy(v.begin(), v.end(), std::ostream_iterator<T>(out, " "));
            out << "]";
        }
        return out;
    }

    template<typename T, int L> std::ostream &operator<<(std::ostream &out, const Eigen::array<T, L> &v) {
        if(!v.empty()) {
            out << "[ ";
            std::copy(v.begin(), v.end(), std::ostream_iterator<T>(out, " "));
            out << "]";
        }
        return out;
    }

    template<typename T> std::ostream &operator<<(std::ostream &out, const std::vector<T> &v) {
        if(!v.empty()) {
            out << "[ ";
            std::copy(v.begin(), v.end(), std::ostream_iterator<T>(out, " "));
            out << "]";
        }
        return out;
    }

}
