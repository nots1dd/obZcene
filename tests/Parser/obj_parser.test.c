#include "Obj/parser.h"
#include "obz_log.h"

DECLARE_OBZ_GLOBAL_LOGGER();

static void obz_test_obj_loader()
{
    OBZ_LOG_INFO(NULL, "Testing OBJ loader...");

    const char* test_file = "cube.obj";
    FILE* f = fopen(test_file,"w");
    if (!f) { OBZ_LOG_ERROR(NULL,"Failed to create test OBJ file"); return; }

    const char* cube_obj = 
"mtllib cube.mtl\n"
"o Cube\n"
"v 1.000000 -1.000000 -1.000000\n"
"v 1.000000 -1.000000 1.000000\n"
"v -1.000000 -1.000000 1.000000\n"
"v -1.000000 -1.000000 -1.000000\n"
"v 1.000000 1.000000 -0.999999\n"
"v 0.999999 1.000000 1.000001\n"
"v -1.000000 1.000000 1.000000\n"
"v -1.000000 1.000000 -1.000000\n"
"vt 1.000000 0.333333\n"
"vt 1.000000 0.666667\n"
"vt 0.666667 0.666667\n"
"vt 0.666667 0.333333\n"
"vt 0.666667 0.000000\n"
"vt 0.000000 0.333333\n"
"vt 0.000000 0.000000\n"
"vt 0.333333 0.000000\n"
"vt 0.333333 1.000000\n"
"vt 0.000000 1.000000\n"
"vt 0.000000 0.666667\n"
"vt 0.333333 0.333333\n"
"vt 0.333333 0.666667\n"
"vt 1.000000 0.000000\n"
"vn 0.000000 -1.000000 0.000000\n"
"vn 0.000000 1.000000 0.000000\n"
"vn 1.000000 0.000000 0.000000\n"
"vn -0.000000 0.000000 1.000000\n"
"vn -1.000000 -0.000000 -0.000000\n"
"vn 0.000000 0.000000 -1.000000\n"
"usemtl Material\n"
"s off\n"
"f 2/1/1 3/2/1 4/3/1\n"
"f 8/1/2 7/4/2 6/5/2\n"
"f 5/6/3 6/7/3 2/8/3\n"
"f 6/8/4 7/5/4 3/4/4\n"
"f 3/9/5 7/10/5 8/11/5\n"
"f 1/12/6 4/13/6 8/11/6\n"
"f 1/4/1 2/1/1 4/3/1\n"
"f 5/14/2 8/1/2 6/5/2\n"
"f 1/12/3 5/6/3 2/8/3\n"
"f 2/12/4 6/8/4 3/4/4\n"
"f 4/13/5 3/9/5 8/11/5\n"
"f 5/6/6 1/12/6 8/11/6\n";

    fwrite(cube_obj,1,strlen(cube_obj),f);
    fclose(f);

    OBZ_ObjMesh mesh;
    OBZ_Result res = obz_obj_load(test_file,&mesh);
    if (res != OBZ_OK)
    {
        OBZ_LOG_ERROR(NULL,"OBJ load failed");
        return;
    }

    OBZ_LOG_INFO(NULL,"Test OBJ loaded successfully: Vertices=%zu, Texcoords=%zu, Normals=%zu, Triangles=%zu",
                 mesh.positions.size,
                 mesh.texcoords.size,
                 mesh.normals.size,
                 mesh.faces.size/3);

    assert(mesh.positions.size==8);
    assert(mesh.texcoords.size==14);
    assert(mesh.normals.size==6);
    assert(mesh.faces.size==36); // 12 triangles * 3 indices

    obz_objmesh_free(&mesh);
    OBZ_LOG_INFO(NULL,"OBJ loader test completed successfully");
}

int main() 
{
    OBZ_Logger* logger = obz_logger_init(NULL, OBZ_LOG_TRACE);

    obz_set_global_logger(logger);

    obz_test_obj_loader();
    return 0;
}
