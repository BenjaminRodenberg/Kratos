//    |  /           |
//    ' /   __| _` | __|  _ \   __|
//    . \  |   (   | |   (   |\__ `
//   _|\_\_|  \__,_|\__|\___/ ____/
//                   Multi-Physics
//
//  License:		 BSD License
//					 Kratos default license: kratos/license.txt
//
//  Main authors:    Reza Najian Asl
//

// System includes


// External includes


// Project includes
#include "custom_elements/helmholtz_surf_prism_element.h"
#include "../StructuralMechanicsApplication/custom_utilities/shellt3_local_coordinate_system.hpp"
#include "includes/checks.h"
#include "includes/define.h"
#include "utilities/math_utils.h"

namespace Kratos
{

//************************************************************************************
//************************************************************************************
HelmholtzSurfPrismElement::HelmholtzSurfPrismElement(IndexType NewId, GeometryType::Pointer pGeometry)
    : Element(NewId, pGeometry)
{
    //DO NOT ADD DOFS HERE!!!

}

//************************************************************************************
//************************************************************************************
HelmholtzSurfPrismElement::HelmholtzSurfPrismElement(IndexType NewId, GeometryType::Pointer pGeometry,  PropertiesType::Pointer pProperties)
    : Element(NewId, pGeometry, pProperties)
{
}

Element::Pointer HelmholtzSurfPrismElement::Create(IndexType NewId, NodesArrayType const& ThisNodes,  PropertiesType::Pointer pProperties) const
{
    return Kratos::make_intrusive<HelmholtzSurfPrismElement>(NewId, GetGeometry().Create(ThisNodes), pProperties);
}

Element::Pointer HelmholtzSurfPrismElement::Create(IndexType NewId, GeometryType::Pointer pGeom,  PropertiesType::Pointer pProperties) const
{
    return Kratos::make_intrusive<HelmholtzSurfPrismElement>(NewId, pGeom, pProperties);
}

HelmholtzSurfPrismElement::~HelmholtzSurfPrismElement()
{
}
/***********************************************************************************/
/***********************************************************************************/

void HelmholtzSurfPrismElement::Calculate(const Variable<Matrix>& rVariable, Matrix& rOutput, const ProcessInfo& rCurrentProcessInfo)
{
    if (rVariable == HELMHOLTZ_MASS_MATRIX)
        CalculateSurfaceMassMatrix(rOutput,rCurrentProcessInfo);

}
//************************************************************************************
//************************************************************************************
void HelmholtzSurfPrismElement::CalculateLocalSystem(MatrixType& rLeftHandSideMatrix,
                                            VectorType& rRightHandSideVector,
                                            const ProcessInfo& rCurrentProcessInfo)
{
    KRATOS_TRY

    auto& r_geometry = this->GetGeometry();
    const SizeType number_of_nodes = r_geometry.size();
    const SizeType dimension = r_geometry.WorkingSpaceDimension();

    // Resizing as needed the LHS
    const SizeType mat_size = number_of_nodes * dimension;

    if ( rLeftHandSideMatrix.size1() != mat_size )
        rLeftHandSideMatrix.resize( mat_size, mat_size, false );

    noalias( rLeftHandSideMatrix ) = ZeroMatrix( mat_size, mat_size ); //resetting LHS

    // Resizing as needed the RHS 
    if ( rRightHandSideVector.size() != mat_size )
        rRightHandSideVector.resize( mat_size, false );

    rRightHandSideVector = ZeroVector( mat_size ); //resetting RHS

    MatrixType M;
    CalculateSurfaceMassMatrix(M,rCurrentProcessInfo);
    MatrixType A;
    CalculateSurfaceStiffnessMatrix(A,rCurrentProcessInfo);

    MatrixType K = A + M;

    const unsigned int number_of_points = r_geometry.size();
    Vector nodal_vals(number_of_points*dimension);
    for(unsigned int node_element = 0; node_element<number_of_points; node_element++)
    {
        const VectorType &source = r_geometry[node_element].FastGetSolutionStepValue(HELMHOLTZ_SOURCE);
        auto node_weight = r_geometry[node_element].GetValue(NUMBER_OF_NEIGHBOUR_ELEMENTS);
        nodal_vals[3 * node_element + 0] = source[0]/node_weight;
        nodal_vals[3 * node_element + 1] = source[1]/node_weight;
        nodal_vals[3 * node_element + 2] = source[2]/node_weight;
    } 

    noalias(rLeftHandSideMatrix) += K;
    noalias(rRightHandSideVector) += nodal_vals;

    //apply drichlet BC
    Vector temp;
    GetValuesVector(temp,0);    
    noalias(rRightHandSideVector) -= prod(rLeftHandSideMatrix,temp);    

    KRATOS_CATCH("")
}

//************************************************************************************
//************************************************************************************
void HelmholtzSurfPrismElement::CalculateLeftHandSide(MatrixType& rLeftHandSideMatrix, const ProcessInfo& rCurrentProcessInfo)
{
    VectorType temp(0);
    CalculateLocalSystem(rLeftHandSideMatrix, temp, rCurrentProcessInfo);
}

//************************************************************************************
//************************************************************************************
void HelmholtzSurfPrismElement::CalculateRightHandSide(VectorType& rRightHandSideVector, const ProcessInfo& rCurrentProcessInfo)
{
    MatrixType temp(0,0);
    CalculateLocalSystem(temp, rRightHandSideVector, rCurrentProcessInfo);
}

//************************************************************************************
//************************************************************************************
void HelmholtzSurfPrismElement::EquationIdVector(EquationIdVectorType& rResult, const ProcessInfo& rCurrentProcessInfo) const
{
    KRATOS_TRY;

    const SizeType number_of_nodes = GetGeometry().size();
    const SizeType dimension = GetGeometry().WorkingSpaceDimension();

    if (rResult.size() != dimension * number_of_nodes)
        rResult.resize(dimension * number_of_nodes,false);

    const SizeType pos = this->GetGeometry()[0].GetDofPosition(HELMHOLTZ_VARS_X);

    if(dimension == 2) {
        for (IndexType i = 0; i < number_of_nodes; ++i) {
            const SizeType index = i * 2;
            rResult[index] = GetGeometry()[i].GetDof(HELMHOLTZ_VARS_X,pos).EquationId();
            rResult[index + 1] = GetGeometry()[i].GetDof(HELMHOLTZ_VARS_Y,pos+1).EquationId();
        }
    } else {
        for (IndexType i = 0; i < number_of_nodes; ++i) {
            const SizeType index = i * 3;
            rResult[index] = GetGeometry()[i].GetDof(HELMHOLTZ_VARS_X,pos).EquationId();
            rResult[index + 1] = GetGeometry()[i].GetDof(HELMHOLTZ_VARS_Y,pos+1).EquationId();
            rResult[index + 2] = GetGeometry()[i].GetDof(HELMHOLTZ_VARS_Z,pos+2).EquationId();
        }
    }

    KRATOS_CATCH("")
}

//************************************************************************************
//************************************************************************************
void HelmholtzSurfPrismElement::GetDofList(DofsVectorType& rElementalDofList,const ProcessInfo& rCurrentProcessInfo) const
{

    KRATOS_TRY;

    const SizeType number_of_nodes = GetGeometry().size();
    const SizeType dimension = GetGeometry().WorkingSpaceDimension();
    rElementalDofList.resize(0);
    rElementalDofList.reserve(dimension*number_of_nodes);

    if(dimension == 2) {
        for (IndexType i = 0; i < number_of_nodes; ++i) {
            rElementalDofList.push_back(GetGeometry()[i].pGetDof(HELMHOLTZ_VARS_X));
            rElementalDofList.push_back( GetGeometry()[i].pGetDof(HELMHOLTZ_VARS_Y));
        }
    } else {
        for (IndexType i = 0; i < number_of_nodes; ++i) {
            rElementalDofList.push_back(GetGeometry()[i].pGetDof(HELMHOLTZ_VARS_X));
            rElementalDofList.push_back( GetGeometry()[i].pGetDof(HELMHOLTZ_VARS_Y));
            rElementalDofList.push_back( GetGeometry()[i].pGetDof(HELMHOLTZ_VARS_Z));
        }
    }

    KRATOS_CATCH("")

}
//******************************************************************************
//******************************************************************************
void HelmholtzSurfPrismElement::GetValuesVector(VectorType &rValues,
                                            int Step) const {
  const GeometryType &rgeom = this->GetGeometry();
  const SizeType num_nodes = rgeom.PointsNumber();
  const unsigned int dimension = GetGeometry().WorkingSpaceDimension();
  const unsigned int local_size = num_nodes * dimension;

  if (rValues.size() != local_size)
    rValues.resize(local_size, false);

  if (dimension == 2) {
    SizeType index = 0;
    for (SizeType i_node = 0; i_node < num_nodes; ++i_node) {
      rValues[index++] =
          rgeom[i_node].FastGetSolutionStepValue(HELMHOLTZ_VARS_X, Step);
      rValues[index++] =
          rgeom[i_node].FastGetSolutionStepValue(HELMHOLTZ_VARS_Y, Step);
    }
  } else if (dimension == 3) {
    SizeType index = 0;
    for (SizeType i_node = 0; i_node < num_nodes; ++i_node) {
      rValues[index++] =
          rgeom[i_node].FastGetSolutionStepValue(HELMHOLTZ_VARS_X, Step);
      rValues[index++] =
          rgeom[i_node].FastGetSolutionStepValue(HELMHOLTZ_VARS_Y, Step);
      rValues[index++] =
          rgeom[i_node].FastGetSolutionStepValue(HELMHOLTZ_VARS_Z, Step);
    }
  }
}
//************************************************************************************
//************************************************************************************
int HelmholtzSurfPrismElement::Check(const ProcessInfo& rCurrentProcessInfo) const
{
    KRATOS_TRY;

    int check = Element::Check(rCurrentProcessInfo);

    const auto& r_geometry = GetGeometry();
    const SizeType number_of_nodes = r_geometry.size();

    // Check that the element's nodes contain all required SolutionStepData and Degrees of freedom
    for ( IndexType i = 0; i < number_of_nodes; i++ ) {
        const NodeType &rnode = r_geometry[i];
        KRATOS_CHECK_VARIABLE_IN_NODAL_DATA(HELMHOLTZ_VARS,rnode)

        KRATOS_CHECK_DOF_IN_NODE(HELMHOLTZ_VARS_X, rnode)
        KRATOS_CHECK_DOF_IN_NODE(HELMHOLTZ_VARS_Y, rnode)
        KRATOS_CHECK_DOF_IN_NODE(HELMHOLTZ_VARS_Z, rnode)
    }

    return check;

    KRATOS_CATCH( "" );
}
/***********************************************************************************/
/***********************************************************************************/

void HelmholtzSurfPrismElement::CalculateSurfaceMassMatrix(
    MatrixType& rMassMatrix,
    const ProcessInfo& rCurrentProcessInfo
    ) const
{
    KRATOS_TRY;

    const auto& r_geom = GetGeometry();
    SizeType dimension = r_geom.WorkingSpaceDimension();
    SizeType number_of_nodes = r_geom.size();
    SizeType mat_size = dimension * number_of_nodes;

    // Clear matrix
    if (rMassMatrix.size1() != mat_size || rMassMatrix.size2() != mat_size)
        rMassMatrix.resize( mat_size, mat_size, false );
    rMassMatrix = ZeroMatrix( mat_size, mat_size );


    Matrix J0(dimension, dimension);

    const IntegrationMethod& integration_method = r_geom.GetDefaultIntegrationMethod();
    // GeometryData::IntegrationMethod::GI_GAUSS_4;
    const GeometryType::IntegrationPointsArrayType& integration_points = r_geom.IntegrationPoints( integration_method );

    MatrixType Ncontainer;
    GetTetrahedraShapeFunctionsValues(Ncontainer,integration_method,rCurrentProcessInfo);    


    for ( IndexType point_number = 0; point_number < integration_points.size(); ++point_number ) {
        const double detJ0 = r_geom.DeterminantOfJacobian(point_number,integration_method);
        const double integration_weight = integration_points[point_number].Weight() * detJ0;
        const Vector& rN = row(Ncontainer,point_number);

        for ( IndexType i = 0; i < number_of_nodes; ++i ) {
            const SizeType index_i = i * dimension;

            for ( IndexType j = 0; j < number_of_nodes; ++j ) {
                const SizeType index_j = j * dimension;
                const double NiNj_weight = rN[i] * rN[j] * integration_weight;

                for ( IndexType k = 0; k < dimension; ++k )
                    rMassMatrix( index_i + k, index_j + k ) += NiNj_weight;
            }
        }
    }   

    KRATOS_CATCH("");
}

void HelmholtzSurfPrismElement::GetPrismShapeFunctionsValues(
    MatrixType& rNMatrix,
    const IntegrationMethod& rIntegrationMethod,
    const ProcessInfo& rCurrentProcessInfo
    ) const
{
    KRATOS_TRY;

    const auto& r_geom = GetGeometry();
    SizeType number_of_nodes = r_geom.size();
    const GeometryType::IntegrationPointsArrayType& integration_points = r_geom.IntegrationPoints( rIntegrationMethod );
    SizeType mat_size1 = integration_points.size();
    SizeType mat_size2 = number_of_nodes;

    rNMatrix.resize( mat_size1, mat_size2, false );
    rNMatrix = ZeroMatrix( mat_size1, mat_size2 );

    // create a prism here 
    double thickness = 0.000001;
    const auto triangle_points = r_geom.Points();
    PointPtrType p0 = new PointType(triangle_points[0].Id(),triangle_points[0]);
    PointPtrType p1 = new PointType(triangle_points[1].Id(),triangle_points[1]);
    PointPtrType p2 = new PointType(triangle_points[2].Id(),triangle_points[2]);
    PointPtrType p3 = new PointType(triangle_points[0].Id()+3,p0->Coordinates() + thickness * r_geom[0].FastGetSolutionStepValue(NORMAL)/norm_2(r_geom[0].FastGetSolutionStepValue(NORMAL)));
    PointPtrType p4 = new PointType(triangle_points[1].Id()+3,p1->Coordinates() + thickness * r_geom[1].FastGetSolutionStepValue(NORMAL)/norm_2(r_geom[1].FastGetSolutionStepValue(NORMAL)));
    PointPtrType p5 = new PointType(triangle_points[2].Id()+3,p2->Coordinates() + thickness * r_geom[2].FastGetSolutionStepValue(NORMAL)/norm_2(r_geom[2].FastGetSolutionStepValue(NORMAL)));
    PrismGeometryType pseudo_prism(p0,p1,p2,p3,p4,p5);

    for ( IndexType point_number = 0; point_number < integration_points.size(); ++point_number ) {    
        Point gp_local_pt = Point(integration_points[point_number].Coordinates());
        Point gp_global_pt;
        r_geom.GlobalCoordinates(gp_global_pt,gp_local_pt);
        Point elem_gp_local_pt;
        pseudo_prism.PointLocalCoordinates(elem_gp_local_pt,gp_global_pt);

        for(IndexType i = 0; i < 3; ++i )
            rNMatrix(point_number,i) = pseudo_prism.ShapeFunctionValue(i,elem_gp_local_pt);
    }
    
    KRATOS_CATCH("");
}

void HelmholtzSurfPrismElement::GetTetrahedraShapeFunctionsValues(
    MatrixType& rNMatrix,
    const IntegrationMethod& rIntegrationMethod,
    const ProcessInfo& rCurrentProcessInfo
    ) const
{
    KRATOS_TRY;

    const auto& r_geom = GetGeometry();
    SizeType number_of_nodes = r_geom.size();
    const GeometryType::IntegrationPointsArrayType& integration_points = r_geom.IntegrationPoints( rIntegrationMethod );
    SizeType mat_size1 = integration_points.size();
    SizeType mat_size2 = number_of_nodes;

    rNMatrix.resize( mat_size1, mat_size2, false );
    rNMatrix = ZeroMatrix( mat_size1, mat_size2 );

    // create a tetrahedra here 
    double height = r_geom.Length();
    const auto triangle_points = r_geom.Points();
    VectorType n_surf;
    CalculateNormal(n_surf);
    PointPtrType p0 = new PointType(triangle_points[0].Id(),triangle_points[0]);
    PointPtrType p1 = new PointType(triangle_points[1].Id(),triangle_points[1]);
    PointPtrType p2 = new PointType(triangle_points[2].Id(),triangle_points[2]);
    PointPtrType p3 = new PointType(triangle_points[0].Id()+3,((p0->Coordinates()+p1->Coordinates()+p2->Coordinates())/3.0) + height * n_surf);
    TetrahedraGeometryType pseudo_tetrahedra(p0,p1,p2,p3);

    for ( IndexType point_number = 0; point_number < integration_points.size(); ++point_number ) {    
        Point gp_local_pt = Point(integration_points[point_number].Coordinates());
        Point gp_global_pt;
        r_geom.GlobalCoordinates(gp_global_pt,gp_local_pt);
        Point elem_gp_local_pt;
        pseudo_tetrahedra.PointLocalCoordinates(elem_gp_local_pt,gp_global_pt);

        for(IndexType i = 0; i < 3; ++i )
            rNMatrix(point_number,i) = pseudo_tetrahedra.ShapeFunctionValue(i,elem_gp_local_pt);

    }  
    KRATOS_CATCH("");
}

/***********************************************************************************/
/***********************************************************************************/

void HelmholtzSurfPrismElement::CalculateSurfaceStiffnessMatrix(
    MatrixType& rStiffnessMatrix,
    const ProcessInfo& rCurrentProcessInfo
    ) const
{
    KRATOS_TRY;

    const auto& r_prop = GetProperties();

    // Checking radius
    KRATOS_ERROR_IF_NOT(r_prop.Has(HELMHOLTZ_RADIUS)) << "HELMHOLTZ_RADIUS has to be provided for the calculations of the HelmholtzSurfPrismElement!" << std::endl;

    const auto& r_geom = GetGeometry();
    SizeType dimension = r_geom.WorkingSpaceDimension();
    SizeType number_of_nodes = r_geom.size();
    SizeType mat_size = dimension * number_of_nodes;

    // Clear matrix
    if (rStiffnessMatrix.size1() != mat_size || rStiffnessMatrix.size2() != mat_size)
        rStiffnessMatrix.resize( mat_size, mat_size, false );
    rStiffnessMatrix = ZeroMatrix( mat_size, mat_size );


    //reading integration points and local gradients
    const IntegrationMethod& integration_method = r_geom.GetDefaultIntegrationMethod();
    const GeometryType::IntegrationPointsArrayType& integration_points = r_geom.IntegrationPoints(integration_method);
    const unsigned int NumGauss = integration_points.size();
    Vector GaussPtsJDet = ZeroVector(NumGauss);
    r_geom.DeterminantOfJacobian(GaussPtsJDet, integration_method);    

    VectorType n_surf;
    CalculateNormal(n_surf);
    MatrixType id_matrix = IdentityMatrix(dimension,dimension);
    MatrixType tangent_projection_matrix = id_matrix - outer_prod(n_surf, n_surf);

    if(false){
        for(std::size_t i_point = 0; i_point<integration_points.size(); ++i_point)
        {
            Matrix DN_DX;
            CalculateTetrahedraDN_DXMatrix(DN_DX,integration_method,i_point,rCurrentProcessInfo);
            MatrixType DN_DX_t = prod(DN_DX,tangent_projection_matrix);

            MatrixType B;
            CalculateBMatrix(B,DN_DX_t,integration_method,i_point);
            
            MatrixType C;
            CalculateCMatrix(C,integration_method,i_point);

            const double r_helmholtz = r_prop[HELMHOLTZ_RADIUS];
            const double IntToReferenceWeight = integration_points[i_point].Weight() * GaussPtsJDet[i_point];

            noalias(rStiffnessMatrix) += r_helmholtz * r_helmholtz * prod(trans(B), IntToReferenceWeight * Matrix(prod(C, B)));
            
        }
    }
    else{
            MatrixType A_dirc = ZeroMatrix(number_of_nodes,number_of_nodes);
            for(std::size_t i_point = 0; i_point<integration_points.size(); ++i_point)
            {
                Matrix DN_DX;
                CalculateTetrahedraDN_DXMatrix(DN_DX,integration_method,i_point,rCurrentProcessInfo);
                const double IntToReferenceWeight = integration_points[i_point].Weight() * GaussPtsJDet[i_point];

                MatrixType DN_DX_t = prod(DN_DX,tangent_projection_matrix);

                const double r_helmholtz = r_prop[HELMHOLTZ_RADIUS];
                noalias(A_dirc) += IntToReferenceWeight * r_helmholtz * r_helmholtz * prod(DN_DX_t, trans(DN_DX_t));
                
            }

            //contruct the stifness matrix in all dims
            for(IndexType i=0;i<number_of_nodes;i++)
                for(IndexType j=0;j<dimension;j++)
                    for(IndexType k=0;k<number_of_nodes;k++)
                        rStiffnessMatrix(dimension*i+j,dimension*k+j) = A_dirc(i,k);
    }

    KRATOS_CATCH("");
}

void HelmholtzSurfPrismElement::CalculatePrismDN_DXMatrix(
    MatrixType& rDN_DX,
    const IntegrationMethod& rIntegrationMethod,
    const IndexType PointNumber,
    const ProcessInfo& rCurrentProcessInfo
    ) const
{
    KRATOS_TRY;

    const auto& r_geom = GetGeometry();

    rDN_DX.resize( 3, 3, false );
    rDN_DX = ZeroMatrix( 3, 3 );    

    // create a prism here 
    double thickness = r_geom.Length();    
    const auto triangle_points = r_geom.Points();
    PointPtrType p0 = new PointType(triangle_points[0].Id(),triangle_points[0]);
    PointPtrType p1 = new PointType(triangle_points[1].Id(),triangle_points[1]);
    PointPtrType p2 = new PointType(triangle_points[2].Id(),triangle_points[2]);
    PointPtrType p3 = new PointType(triangle_points[0].Id()+3,p0->Coordinates() + thickness * r_geom[0].FastGetSolutionStepValue(NORMAL)/norm_2(r_geom[0].FastGetSolutionStepValue(NORMAL)));
    PointPtrType p4 = new PointType(triangle_points[1].Id()+3,p1->Coordinates() + thickness * r_geom[1].FastGetSolutionStepValue(NORMAL)/norm_2(r_geom[1].FastGetSolutionStepValue(NORMAL)));
    PointPtrType p5 = new PointType(triangle_points[2].Id()+3,p2->Coordinates() + thickness * r_geom[2].FastGetSolutionStepValue(NORMAL)/norm_2(r_geom[2].FastGetSolutionStepValue(NORMAL)));
    PrismGeometryType pseudo_prism(p0,p1,p2,p3,p4,p5);

    const GeometryType::IntegrationPointsArrayType& integration_points = r_geom.IntegrationPoints(rIntegrationMethod);
    Point surf_gp_local_pt = Point(integration_points[PointNumber].Coordinates());
    Point surf_gp_global_pt;
    r_geom.GlobalCoordinates(surf_gp_global_pt,surf_gp_local_pt);
    Point elem_surf_gp_local_pt;
    pseudo_prism.PointLocalCoordinates(elem_surf_gp_local_pt,surf_gp_global_pt);    

    MatrixType DN_De;
    pseudo_prism.ShapeFunctionsLocalGradients(DN_De,elem_surf_gp_local_pt);

    MatrixType InvJ0;
    pseudo_prism.InverseOfJacobian(InvJ0,elem_surf_gp_local_pt);

    MatrixType elem_DN_DX = prod(DN_De,InvJ0);

    for(IndexType i = 0; i<3; i++)
        for(IndexType j = 0; j<3; j++)
            rDN_DX(i,j) = elem_DN_DX(i,j);   
     
    KRATOS_CATCH("");
}

void HelmholtzSurfPrismElement::CalculateTetrahedraDN_DXMatrix(
    MatrixType& rDN_DX,
    const IntegrationMethod& rIntegrationMethod,
    const IndexType PointNumber,
    const ProcessInfo& rCurrentProcessInfo
    ) const
{
    KRATOS_TRY;

    const auto& r_geom = GetGeometry();

    rDN_DX.resize( 3, 3, false );
    rDN_DX = ZeroMatrix( 3, 3 );    

    // create a tetrahedra here
    double height = r_geom.Length();
    const auto triangle_points = r_geom.Points();
    VectorType n_surf;
    CalculateNormal(n_surf);
    PointPtrType p0 = new PointType(triangle_points[0].Id(),triangle_points[0]);
    PointPtrType p1 = new PointType(triangle_points[1].Id(),triangle_points[1]);
    PointPtrType p2 = new PointType(triangle_points[2].Id(),triangle_points[2]);
    PointPtrType p3 = new PointType(triangle_points[0].Id()+3,((p0->Coordinates()+p1->Coordinates()+p2->Coordinates())/3.0) + height * n_surf);
    TetrahedraGeometryType pseudo_tetrahedra(p0,p1,p2,p3);

    const GeometryType::IntegrationPointsArrayType& integration_points = r_geom.IntegrationPoints(rIntegrationMethod);
    Point surf_gp_local_pt = Point(integration_points[PointNumber].Coordinates());
    Point surf_gp_global_pt;
    r_geom.GlobalCoordinates(surf_gp_global_pt,surf_gp_local_pt);
    Point elem_surf_gp_local_pt;
    pseudo_tetrahedra.PointLocalCoordinates(elem_surf_gp_local_pt,surf_gp_global_pt);    

    MatrixType DN_De;
    pseudo_tetrahedra.ShapeFunctionsLocalGradients(DN_De,elem_surf_gp_local_pt);

    MatrixType InvJ0;
    pseudo_tetrahedra.InverseOfJacobian(InvJ0,elem_surf_gp_local_pt);

    MatrixType elem_DN_DX = prod(DN_De,InvJ0);

    for(IndexType i = 0; i<3; i++)
        for(IndexType j = 0; j<3; j++)
            rDN_DX(i,j) = elem_DN_DX(i,j);     
     
    KRATOS_CATCH("");
}

void HelmholtzSurfPrismElement::CalculateRotationMatrix(
    MatrixType& rRotMatrix,
    const ProcessInfo& rCurrentProcessInfo
    ) const
{
    KRATOS_TRY;

    rRotMatrix.resize(9,9,false);
    noalias(rRotMatrix) = ZeroMatrix(9, 9);

    const auto& r_geom = GetGeometry();    
    ShellT3_LocalCoordinateSystem LCS(r_geom[0].Coordinates(),
                                      r_geom[1].Coordinates(),
                                      r_geom[2].Coordinates());

    const Matrix& rOrientation = LCS.Orientation();                           


    for (size_t k = 0; k < 3; k++) {
        size_t i = k * 3;
        rRotMatrix(i  , i) = rOrientation(0, 0);
        rRotMatrix(i  , i+1) = rOrientation(0, 1);
        rRotMatrix(i  , i+2) = rOrientation(0, 2);
        rRotMatrix(i+1, i) = rOrientation(1, 0);
        rRotMatrix(i+1, i+1) = rOrientation(1, 1);
        rRotMatrix(i+1, i+2) = rOrientation(1, 2);
        rRotMatrix(i+2, i) = rOrientation(2, 0);
        rRotMatrix(i+2, i+1) = rOrientation(2, 1);
        rRotMatrix(i+2, i+2) = rOrientation(2, 2);
    }

    KRATOS_CATCH("");
}

void HelmholtzSurfPrismElement::CalculateNormal(VectorType & r_n) const
{
    const auto& r_cond_geom = GetGeometry();

    array_1d<double,3> v1,v2;
    v1[0] = r_cond_geom[1].X() - r_cond_geom[0].X();
    v1[1] = r_cond_geom[1].Y() - r_cond_geom[0].Y();
    v1[2] = r_cond_geom[1].Z() - r_cond_geom[0].Z();

    v2[0] = r_cond_geom[2].X() - r_cond_geom[0].X();
    v2[1] = r_cond_geom[2].Y() - r_cond_geom[0].Y();
    v2[2] = r_cond_geom[2].Z() - r_cond_geom[0].Z();

    r_n.resize(3);
    MathUtils<double>::CrossProduct(r_n,v1,v2);
    double norm = MathUtils<double>::Norm3(r_n);
    r_n /= norm;
}

void HelmholtzSurfPrismElement::CalculateCMatrix(MatrixType& rCMatrix, const IntegrationMethod& rIntegrationMethod, const IndexType PointNumber) const 
{
  KRATOS_TRY;

  rCMatrix.resize(6,6,false);
  rCMatrix = ZeroMatrix(6, 6);

  const auto& r_geom = GetGeometry();
  Vector GaussPtsJDet;
  r_geom.DeterminantOfJacobian(GaussPtsJDet, rIntegrationMethod);  

  const double poisson_coefficient = this->pGetProperties()->Has(HELMHOLTZ_POISSON_RATIO)
    ? this->pGetProperties()->GetValue(HELMHOLTZ_POISSON_RATIO) : 0.3;

  // The ratio between lambda and mu affects relative stiffening against
  // volume or shape change.
  const double lambda =  poisson_coefficient / ((1 + poisson_coefficient) * (1 - 2 * poisson_coefficient));
  const double mu = 1.0 / (2 * (1 + poisson_coefficient));

  // stress = lambda*tr(strain tensor)*I + 2*mu*(strain tensor).

    rCMatrix(0, 0) = lambda + 2 * mu;
    rCMatrix(1, 1) = rCMatrix(0, 0);
    rCMatrix(2, 2) = rCMatrix(0, 0);
    rCMatrix(3, 3) = mu;
    rCMatrix(4, 4) = mu;
    rCMatrix(5, 5) = mu;
    rCMatrix(0, 1) = lambda;
    rCMatrix(1, 0) = lambda;
    rCMatrix(0, 2) = lambda;
    rCMatrix(2, 0) = lambda;
    rCMatrix(1, 2) = lambda;
    rCMatrix(2, 1) = lambda;

  KRATOS_CATCH("");
}

void HelmholtzSurfPrismElement::CalculateBMatrix(MatrixType& rBMatrix, const MatrixType& rDN_DX_tMatrix, const IntegrationMethod& rIntegrationMethod, const IndexType PointNumber) const 
{
  KRATOS_TRY;

  const auto& r_geom = GetGeometry();
  const SizeType num_nodes = r_geom.PointsNumber();

  rBMatrix.resize(6,num_nodes * 3,false);
  rBMatrix = ZeroMatrix(6, num_nodes * 3);  


    SizeType index = 0;
    for (SizeType i_node = 0; i_node < num_nodes; ++i_node) {
        rBMatrix(0, index + 0) = rDN_DX_tMatrix(i_node, 0);
        rBMatrix(1, index + 1) = rDN_DX_tMatrix(i_node, 1);
        rBMatrix(2, index + 2) = rDN_DX_tMatrix(i_node, 2);
        rBMatrix(3, index + 0) = rDN_DX_tMatrix(i_node, 1);
        rBMatrix(3, index + 1) = rDN_DX_tMatrix(i_node, 0);
        rBMatrix(4, index + 1) = rDN_DX_tMatrix(i_node, 2);
        rBMatrix(4, index + 2) = rDN_DX_tMatrix(i_node, 1);
        rBMatrix(5, index + 0) = rDN_DX_tMatrix(i_node, 2);
        rBMatrix(5, index + 2) = rDN_DX_tMatrix(i_node, 0);
        index += 3;
    }
  

  KRATOS_CATCH("");
}

} // Namespace Kratos
