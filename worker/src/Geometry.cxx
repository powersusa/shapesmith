#include "Geometry.h"
#include "Tesselate.h"
#include "Transform.h"
#include "Util.h"


TopoDS_Shape Shape::shape() {
    return shape_;
}

void Shape::ApplyOrigin(map< string, mValue > json) {
    if (!json["origin"].is_null()) {
        map< string, mValue > origin = json["origin"].get_obj();
        double x = Util::to_d(origin["x"]);
        double y = Util::to_d(origin["y"]);
        double z = Util::to_d(origin["z"]);
        gp_Trsf transformation = gp_Trsf();
        transformation.SetTranslation(gp_Vec(x,y,z));
        
        BRepBuilderAPI_Transform brep_transform(shape_, transformation);
        shape_ = brep_transform.Shape();
    }
}

void Shape::ApplyTransform(map< string, mValue > transformJson) {
    string transformType = transformJson["type"].get_str();
    map< string, mValue > origin = transformJson["origin"].get_obj();
    map< string, mValue > parameters = transformJson["parameters"].get_obj();
    
    if (transformType == "rotate") {
        auto_ptr< Transformer<Rotate> > transformer(new Transformer<Rotate>(shape_, origin, parameters));
        shape_ = transformer->transformed_shape();
        return;
    } else if (transformType == "scale") {
        auto_ptr< Transformer<Scale> > transformer(new Transformer<Scale>(shape_, origin, parameters));
        shape_ = transformer->transformed_shape();
        return;
    } else if (transformType == "translate") {
        auto_ptr< Transformer<Translate> > transformer(new Transformer<Translate>(shape_, origin, parameters));
        shape_ = transformer->transformed_shape();
        return;
    } else if (transformType == "mirror") {
        auto_ptr< Transformer<Mirror> > transformer(new Transformer<Mirror>(shape_, origin, parameters));
        shape_ = transformer->transformed_shape();
        return;
    }
    throw "transform type not found";
}

void Shape::ApplyTransforms(map< string, mValue > json) {
    if (!json["transforms"].is_null()) {
        mArray transforms = json["transforms"].get_array();
        for (unsigned int k = 0; k < transforms.size(); ++k) {
            mValue transformJson = transforms[k];
            this->ApplyTransform(transformJson.get_obj());
        }
    } 
}

void Shape::ToOriginTransformAndMesh(map< string, mValue > json) {
    this->ApplyOrigin(json);
    this->ApplyTransforms(json);
    this->Mesh();
}

#pragma mark 3D shapes

void Shape3D::Mesh() {
    TopExp_Explorer Ex; 
    int numFaces = 0;
    for (Ex.Init(shape_, TopAbs_FACE); Ex.More(); Ex.Next()) { 
        ++numFaces;
    }
    
    if (numFaces > 0) {
        BRepMesh().Mesh(shape_, 1.0);
    }
}

mValue Shape3D::Tesselate() {
    auto_ptr<Tesselator3D> tesselator(new Tesselator3D(shape_));
    return tesselator->tesselate();
}

Cuboid::Cuboid(map< string, mValue > json) {
    map< string, mValue > parameters = json["parameters"].get_obj();
    double width = Util::to_d(parameters["u"]);
    double depth = Util::to_d(parameters["v"]);
    double height = Util::to_d(parameters["w"]);
	 
    map< string, mValue > origin = json["origin"].get_obj();
	 
    if (width < 0) {
	origin["x"] = Util::to_d(origin["x"]) + width;
	width = -width;
    }

    if (depth < 0) {
	origin["y"] = Util::to_d(origin["y"]) + depth;
	depth = -depth;
    }
	 
    if (height < 0) {
	origin["z"] = Util::to_d(origin["z"]) + height;
	height = -height;
    }
    json["origin"] = origin;
	 
    shape_ = BRepPrimAPI_MakeBox(width, depth, height).Shape();
    ToOriginTransformAndMesh(json);
}

Sphere::Sphere(map< string, mValue > json) {
    map< string, mValue > parameters = json["parameters"].get_obj();
    double radius = Util::to_d(parameters["r"]);
    shape_ = BRepPrimAPI_MakeSphere(radius).Shape();
    ToOriginTransformAndMesh(json);    
}

Cylinder::Cylinder(map< string, mValue > json) {
    map< string, mValue > parameters = json["parameters"].get_obj();
    double r = Util::to_d(parameters["r"]);
    double h = Util::to_d(parameters["h"]);
    
    map< string, mValue > origin = json["origin"].get_obj();
    if(h < 0) {
	origin["z"] = Util::to_d(origin["z"]) + h;
	h = -h;
    }
    json["origin"] = origin;
    
    shape_ = BRepPrimAPI_MakeCylinder(r, h).Shape() ;
    ToOriginTransformAndMesh(json);
}

Cone::Cone(map< string, mValue > json) {
    map< string, mValue > parameters = json["parameters"].get_obj();
    double r1 = Util::to_d(parameters["r1"]);
    double r2 = Util::to_d(parameters["r2"]);
    double h = Util::to_d(parameters["h"]);

    map< string, mValue > origin = json["origin"].get_obj();
    if(h < 0) {
	origin["z"] = Util::to_d(origin["z"]) + h;
	h = -h;
	swap(r1, r2);
    }
    json["origin"] = origin;
        
    shape_ = BRepPrimAPI_MakeCone(r1, r2, h).Shape();
    ToOriginTransformAndMesh(json);
}

Wedge::Wedge(map< string, mValue > json) {
    map< string, mValue > parameters = json["parameters"].get_obj();
    double u1 = Util::to_d(parameters["u1"]);
    double u2 = Util::to_d(parameters["u2"]);
    double v = Util::to_d(parameters["v"]);
    double w = Util::to_d(parameters["w"]);

    map< string, mValue > origin = json["origin"].get_obj();
    if(w < 0) {
	origin["z"] = Util::to_d(origin["z"]) + w;
	w = -w;
    }
    json["origin"] = origin;
    shape_ = BRepPrimAPI_MakeWedge(u1, v, w, u2).Shape();                                                
    ToOriginTransformAndMesh(json);
}


Torus::Torus(map< string, mValue > json) {
    map< string, mValue > parameters = json["parameters"].get_obj();
    double r1 = Util::to_d(parameters["r1"]);
    double r2 = Util::to_d(parameters["r2"]);
    shape_ =BRepPrimAPI_MakeTorus(r1, r2).Shape();
    ToOriginTransformAndMesh(json);
}

#pragma mark 1D Shapes

void Shape1D::Mesh() {
    // Do nothing. Tesselation is done on-demand.
}

mValue Shape1D::Tesselate() {
    auto_ptr<Tesselator1D> tesselator(new Tesselator1D(shape_));
    return tesselator->tesselate();
}

Ellipse1D::Ellipse1D(map< string, mValue > json) {
    gp_Elips ellipse = gp_Elips(gp_Ax2(gp_Pnt(0,0,0),gp_Dir(0,0,1)), 50.0, 20.0);
	shape_ = BRepBuilderAPI_MakeEdge(ellipse, 0, M_PI*2).Edge();
}


#pragma mark Boolean

Boolean::Boolean(map< string, mValue > json, vector<TopoDS_Shape>& shapes, boolean_op function) {
    vector<TopoDS_Shape>::iterator it = shapes.begin();
    shape_ = *it;
    ++it;
    for ( ; it < shapes.end(); ++it ) {
	shape_ = function(*it, shape_);
    }
    ToOriginTransformAndMesh(json);
}

TopoDS_Shape fuse(const TopoDS_Shape& a, const TopoDS_Shape& b) {
    return BRepAlgoAPI_Fuse(a,b);
}

TopoDS_Shape common(const TopoDS_Shape& a, const TopoDS_Shape& b) {
    return BRepAlgoAPI_Common(a,b);
}

TopoDS_Shape cut(const TopoDS_Shape& a, const TopoDS_Shape& b) {
    return BRepAlgoAPI_Cut(a,b);
}

Union::Union(map< string, mValue > json, vector<TopoDS_Shape>& shapes) 
    : Boolean(json, shapes, &fuse) {
}

Intersect::Intersect(map< string, mValue > json, vector<TopoDS_Shape>& shapes) 
    : Boolean(json, shapes, &common) {
}

Subtract::Subtract(map< string, mValue > json, vector<TopoDS_Shape>& shapes) 
    : Boolean(json, shapes, &cut) {
}
