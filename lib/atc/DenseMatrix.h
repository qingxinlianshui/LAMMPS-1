#ifndef DENSEMATRIX_H
#define DENSEMATRIX_H

#include "Matrix.h"

template <typename T>
class DenseMatrix : public Matrix<T>
{
public:
  DenseMatrix(INDEX rows=0, INDEX cols=0, bool z=1) { _create(rows, cols, z); }
  DenseMatrix(const DenseMatrix<T>& c) : _data(NULL){ _copy(c); }
  DenseMatrix(const SparseMatrix<T>& c): _data(NULL){ c.dense_copy(*this);}
  DenseMatrix(const Matrix<T>& c)      : _data(NULL){ _copy(c); }
//  const SparseMatrix<T> * p = sparse_cast(&c);
//  (p) ? p->dense_copy(*this) : _copy(c); }
  ~DenseMatrix()                                    { _delete();}

  void reset (INDEX rows, INDEX cols, bool zero=true);
  void resize(INDEX rows, INDEX cols, bool copy=false);
  void copy (const T * ptr, INDEX rows, INDEX cols);
  /** returns transpose(this) * B */
  DenseMatrix<T> transMat(const DenseMatrix<T>& B) const;
  /** returns by element multiply A_ij = this_ij * B_ij */
  DenseMatrix<T> mult_by_element(const DenseMatrix<T>& B) const;
  
  /** overloaded virtual functions */
  T& operator()(INDEX i, INDEX j)       { MICK(i,j) return DATA(i,j); }
  T  operator()(INDEX i, INDEX j) const { MICK(i,j) return DATA(i,j); }
  T  operator[](INDEX i)          const { VICK(i) return _data[i]; }
  T& operator[](INDEX i)                { VICK(i) return _data[i]; }
  INDEX nRows()                   const { return _nRows; }
  INDEX nCols()                   const { return _nCols; }
  T * get_ptr()                   const { return _data;  }
  void write_restart(FILE *f)     const;
  void set_all_elements_to(const T &v);
  DiagonalMatrix<T> get_diag()    const;
 
  DenseMatrix<T>& operator=(const T &v);
  DenseMatrix<T>& operator=(const Matrix<T> &c);
  DenseMatrix<T>& operator=(const DenseMatrix<T> &c);
  DenseMatrix<T>& operator=(const SparseMatrix<T> &c);

private:
  void _delete();
  void _create(INDEX rows, INDEX cols, bool zero=false);
  void _copy(const Matrix<T> &c);

  T *_data;
  INDEX _nRows, _nCols;
};

//! Computes the cofactor matrix of A.
template<typename T>
DenseMatrix<T> adjugate(const Matrix<T> &A, bool symmetric=false);

//! Returns a the tensor product of two vectors
template<typename T>
DenseMatrix<T> tensor_product(const Vector<T> &a, const Vector<T> &b);

//----------------------------------------------------------------------------
// Returns an identity matrix, defaults to 3x3.
//----------------------------------------------------------------------------
template<typename T>
DenseMatrix<T> eye(INDEX rows=3, INDEX cols=3)
{
  const double dij[] = {0.0, 1.0};
  DENS_MAT I(rows, cols, false);  // do not need to pre-zero
  for (INDEX j=0; j<cols; j++)
    for (INDEX i=0; i<rows; i++)
      I(i,j) = dij[i==j];
  return I;
}
//----------------------------------------------------------------------------
// resizes the matrix and optionally zeros it out (default - zero)
//----------------------------------------------------------------------------
template <typename T>
void DenseMatrix<T>::reset(INDEX rows, INDEX cols, bool zero)
{
  if (!this->is_size(rows, cols))
  {
     _delete();
     _create(rows, cols);
  }
  if (zero) this->zero();
}
//----------------------------------------------------------------------------
// resizes the matrix and optionally copies over what still fits
//----------------------------------------------------------------------------
template <typename T>
void DenseMatrix<T>::resize(INDEX rows, INDEX cols, bool copy)
{
  if (this->is_size(rows, cols)) return;  // if is correct size, done
  if (!copy)
  {
     _delete();
     _create(rows, cols);
     return;
  }
  DenseMatrix<T> temp(*this);
  _delete();
  _create(rows, cols);
  FORij MIDX(i,j) = temp.in_range(i,j) ? temp(i,j) : T(0);
}
//----------------------------------------------------------------------------
// resizes the matrix and copies data
//----------------------------------------------------------------------------
template <typename T>
void DenseMatrix<T>::copy(const T * ptr, INDEX rows, INDEX cols)
{
  resize(rows, cols, false);
  memcpy(_data, ptr, this->size()*sizeof(T));
}
//----------------------------------------------------------------------------
// returns transpose(this) * B
//----------------------------------------------------------------------------
template <typename T>
DenseMatrix<T> DenseMatrix<T>::transMat(const DenseMatrix<T>& B)          const
{
  DenseMatrix C;
  MultAB(*this, B, C, true);
  return C;
}
//----------------------------------------------------------------------------
// returns this_ij * B_ij
//----------------------------------------------------------------------------
template <typename T>
DenseMatrix<T> DenseMatrix<T>::mult_by_element(const DenseMatrix<T>& B)   const
{
  DenseMatrix C;
  C.reset(_nRows,_nCols);
  FORij C(i,j) = (*this)(i,j)*B(i,j);
  return C;
}
//----------------------------------------------------------------------------
// writes the matrix data to a file
//----------------------------------------------------------------------------
template <typename T>
void DenseMatrix<T>::write_restart(FILE *f)                               const
{
  fwrite(&_nRows, sizeof(INDEX),1,f);
  fwrite(&_nCols, sizeof(INDEX),1,f);
  if (this->size()) fwrite(_data, sizeof(T), this->size(), f);
}
//----------------------------------------------------------------------------
// sets all elements to a value (optimized)
//----------------------------------------------------------------------------
template <typename T>
inline void DenseMatrix<T>::set_all_elements_to(const T &v)
{
  FORi _data[i] = v;
}
//-----------------------------------------------------------------------------
// Return a diagonal matrix containing the diagonal entries of this matrix 
//-----------------------------------------------------------------------------
template<typename T>
DiagonalMatrix<T> DenseMatrix<T>::get_diag() const 
{
  DiagonalMatrix<T> D(nRows(), true); // initialized to zero
  INDEX i;
  for (i=0; i<nRows(); i++)
  { 
    D(i,i) = DATA(i,i);
  }
  return D;
}
//----------------------------------------------------------------------------
// clears allocated memory
//----------------------------------------------------------------------------
template <typename T>
void DenseMatrix<T>::_delete()
{
  _nRows = _nCols = 0;
  if (_data) delete [] _data;
}
//----------------------------------------------------------------------------
// allocates memory for an rows by cols DenseMatrix
//----------------------------------------------------------------------------
template <typename T>
void DenseMatrix<T>::_create(INDEX rows, INDEX cols, bool zero)
{
  _nRows=rows; 
  _nCols=cols;
  _data = (this->size() ? new T [_nCols*_nRows] : NULL);
  if (zero) this->zero();
}
//----------------------------------------------------------------------------
// creates a deep memory copy from a general matrix
//----------------------------------------------------------------------------
template <typename T>
void DenseMatrix<T>::_copy(const Matrix<T> &c) 
{
  if (!_data || this->size()!=c.size())
  {
    _delete(); 
    _create(c.nRows(), c.nCols());
  }
  else 
  {
    _nRows = c.nRows();
    _nCols = c.nCols();
  }
  memcpy(_data, c.get_ptr(), c.size()*sizeof(T));
}
//----------------------------------------------------------------------------
// sets all elements to a constant 
//----------------------------------------------------------------------------
template <typename T>
DenseMatrix<T>& DenseMatrix<T>::operator=(const T &v)
{
  this->set_all_elements_to(v);
  return *this;
}
//----------------------------------------------------------------------------
// copys c with a deep copy
//----------------------------------------------------------------------------
template <typename T>
DenseMatrix<T>& DenseMatrix<T>::operator=(const Matrix<T> &c)
{
  _copy(c);
  return *this;
}
//----------------------------------------------------------------------------
// copys c with a deep copy
//----------------------------------------------------------------------------
template <typename T>
DenseMatrix<T>& DenseMatrix<T>::operator=(const DenseMatrix<T> &c)
{
  _copy(c);
  return *this;
}
//-----------------------------------------------------------------------------
// copys c with a deep copy, including zeros
//-----------------------------------------------------------------------------
template <typename T>
DenseMatrix<T>& DenseMatrix<T>::operator=(const SparseMatrix<T> &c)
{
  _delete();
  _create(c.nRows(), c.nCols(), true);
  SparseMatrix<T>::compress(c);
  for (INDEX i=0; i<c.size(); i++)
  {
    TRIPLET<T> x = c.get_triplet(i);
    cout << "x.i: "<< x.i << "\nx.j: "<< x.j << "\nv.j: "<< x.v << std::endl << std::endl;
    MIDX(x.i, x.j) =  x.v;
  }
  return *this;
}

//* Returns the transpose of the cofactor matrix of A.
//* see http://en.wikipedia.org/wiki/Adjugate_matrix 
//* symmetric flag only affects cases N>3 
template<typename T> 
DenseMatrix<T> adjugate(const Matrix<T> &A, bool symmetric)
{
  if (!A.is_square()) gerror("adjugate can only be computed for square matrices.");
  DenseMatrix<T> C(A.nRows(), A.nRows());
  switch (A.nRows()) {
    case 1:
      gerror("adjugate must be computed for matrixes of size greater than 1");
    case 2:   
      C(0,0) = A(1,1);  C(0,1) =-A(0,1);
      C(1,0) =-A(1,0);  C(1,1) = A(0,0);
      break;
    case 3:   // 3x3 case was tested vs matlab
      C(0,0) = A(1,1)*A(2,2)-A(1,2)*A(2,1);
      C(1,0) =-A(1,0)*A(2,2)+A(1,2)*A(2,0);   // i+j is odd (reverse sign)
      C(2,0) = A(1,0)*A(2,1)-A(1,1)*A(2,0);
      C(0,1) =-A(0,1)*A(2,2)+A(0,2)*A(2,1);   // i+j is odd
      C(1,1) = A(0,0)*A(2,2)-A(0,2)*A(2,0);
      C(2,1) =-A(0,0)*A(2,1)+A(0,1)*A(2,0);   // i+j is odd
      C(0,2) = A(0,1)*A(1,2)-A(0,2)*A(1,1);
      C(1,2) =-A(0,0)*A(1,2)+A(0,2)*A(1,0);   // i+j is odd 
      C(2,2) = A(0,0)*A(1,1)-A(0,1)*A(1,0);
      break;
    default:  
      // NOTE: - det() is only defined for type double
      // this feature is neither tested nor optimal - use at your own risk!!!
      DenseMatrix<T> m(A.nRows()-1, A.nRows()-1);
      double sign[] = {1.0, -1.0};
      for (unsigned j=0; j<A.nCols(); j++) {
        for (unsigned i=0; i<A.nRows(); i++) {
          for (unsigned mj=0; mj<m.nCols(); mj++) {
            for (unsigned mi=0; mi<m.nRows(); mi++) { 
              m(mi, mj) = A(mi+(mi>=i), mj+(mj>=j));  // skip row i and col j
            }
          }
          if (!symmetric) C(j,i)=det(m)*sign[(i+j)&1];
          if (symmetric && i>=j) C(i,j)=C(j,i)=det(m)*sign[(i+j)&1];
        }
      }
  }
  return C;
}

// Returns a the tensor product of two vectors
template<typename T>
DenseMatrix<T> tensor_product(const Vector<T> &a, const Vector<T> &b)
{
  DenseMatrix<T> ab(a.size(), b.size());
  for (unsigned j=0; j<b.size(); j++)
    for (unsigned i=0; i<a.size(); i++)
      ab(i,j) = a(i)*b(j);
  return ab;
}

//* Returns a DenseMatrix with random values (like matlab rand(m,n)
template<typename T>
DenseMatrix<T> rand(unsigned rows, unsigned cols, int seed=1234)
{
  srand(seed);
  const double rand_max_inv = 1.0 / double(RAND_MAX);
  DenseMatrix<T> R(rows, cols, false);
  for (unsigned i=0; i<R.size(); i++) R[i]=double(::rand())*rand_max_inv;
  return R;
}
#endif

