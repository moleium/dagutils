#include <math/dag_Point2.h>
#include <math/dag_Point4.h>
#include <math/dag_Point3.h>

#include <math/dag_IPoint2.h>
#include <math/dag_IPoint3.h>
#include <math/dag_Matrix3.h>

const Point2 Point2::ZERO(0, 0);
const Point2 Point2::ONE(1, 1);\

const Point3 Point3::ZERO(0, 0, 0);
const Point3 Point3::ONE(1, 1, 1);

const Point4 Point4::ZERO(0, 0, 0, 0);
const Point4 Point4::ONE(1, 1, 1, 1);

const IPoint2 IPoint2::ZERO(0, 0);
const IPoint2 IPoint2::ONE(1, 1);

const IPoint3 IPoint3::ZERO(0, 0, 0);
const IPoint3 IPoint3::ONE(1, 1, 1);

const Matrix3 Matrix3::IDENT(1.f);
const Matrix3 Matrix3::ZERO(0.f);