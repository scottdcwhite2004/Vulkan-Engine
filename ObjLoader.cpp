#include "ObjLoader.h"





std::tuple<std::vector<Coordinates>, std::vector<ObjTexCoord>, std::vector<Normal>, std::vector<uint16_t>, std::vector<uint16_t>, std::vector<uint16_t>> ObjLoader::loadFile(const std::string& filename) const
{    
    std::ifstream fin(filename);
    if (!fin.is_open()) {
        throw std::runtime_error("ObjLoader: failed to open file: " + filename);
    }

    int objectCount = 0;
    std::vector<object> obj;
    std::vector<Coordinates> coordinates;
    std::vector<ObjTexCoord> textures;
    std::vector<Normal> normals;

    std::string tag;
    while (fin >> tag) {
        if (tag == "o") {
            std::string objectName;
            fin >> objectName;
            obj.emplace_back();
            obj.back().objectNames = objectName;
            ++objectCount;
        }
        else if (tag == "v") {
            Coordinates v{};
            fin >> v.x >> v.y >> v.z;
            coordinates.push_back(v);
        }
        else if (tag == "vt") {
            ObjTexCoord t{};
            fin >> t.u >> t.v;
            textures.push_back(t);
        }
        else if (tag == "vn") {
            Normal n{};
            fin >> n.x >> n.y >> n.z;
            normals.push_back(n);
        }
        else if (tag == "f") {
            std::string line;
            std::getline(fin, line);
            if (obj.empty()) {
                obj.emplace_back();
                ++objectCount;
            }

            std::istringstream sline(line);
            std::vector<uint16_t> vIdx;
            std::vector<uint16_t> tIdx;
            std::vector<uint16_t> nIdx;

            std::string token;
            while (sline >> token) {
                uint32_t vi_u32 = 0, ti_u32 = 0, ni_u32 = 0;
                bool hasVi = false, hasTi = false, hasNi = false;

                size_t firstSlash = token.find('/');
                if (firstSlash == std::string::npos) {
                    // vi
                    vi_u32 = static_cast<uint32_t>(std::stoi(token));
                    hasVi = true;
                }
                else {
                    // s_vi
                    const std::string s_vi = token.substr(0, firstSlash);
                    if (!s_vi.empty()) {
                        vi_u32 = static_cast<uint32_t>(std::stoi(s_vi));
                        hasVi = true;
                    }
                    size_t secondSlash = token.find('/', firstSlash + 1);
                    if (secondSlash == std::string::npos) {
                        // vi/ti
                        const std::string s_ti = token.substr(firstSlash + 1);
                        if (!s_ti.empty()) {
                            ti_u32 = static_cast<uint32_t>(std::stoi(s_ti));
                            hasTi = true;
                        }
                    }
                    else {
                        // vi/ti/ni or vi//ni
                        const std::string s_ti = token.substr(firstSlash + 1, secondSlash - firstSlash - 1);
                        const std::string s_ni = token.substr(secondSlash + 1);
                        if (!s_ti.empty()) {
                            ti_u32 = static_cast<uint32_t>(std::stoi(s_ti));
                            hasTi = true;
                        }
                        if (!s_ni.empty()) {
                            ni_u32 = static_cast<uint32_t>(std::stoi(s_ni));
                            hasNi = true;
                        }
                    }
                }

                const uint16_t vi = hasVi ? static_cast<uint16_t>(vi_u32 - 1) : UINT16_MAX;
                const uint16_t ti = hasTi ? static_cast<uint16_t>(ti_u32 - 1) : UINT16_MAX;
                const uint16_t ni = hasNi ? static_cast<uint16_t>(ni_u32 - 1) : UINT16_MAX;

                vIdx.push_back(vi);
                tIdx.push_back(ti);
                nIdx.push_back(ni);
            }

            for (size_t j = 1; j + 1 < vIdx.size(); ++j) {
                obj.back().vertexIndices.push_back(vIdx[0]);
                obj.back().vertexIndices.push_back(vIdx[j]);
                obj.back().vertexIndices.push_back(vIdx[j + 1]);

                obj.back().textureIndices.push_back(tIdx[0]);
                obj.back().textureIndices.push_back(tIdx[j]);
                obj.back().textureIndices.push_back(tIdx[j + 1]);

                obj.back().normalIndices.push_back(nIdx[0]);
                obj.back().normalIndices.push_back(nIdx[j]);
                obj.back().normalIndices.push_back(nIdx[j + 1]);
            }
        }
    }

    std::vector<uint16_t> allVertexIndices;
    std::vector<uint16_t> allTextureIndices;
    std::vector<uint16_t> allNormalIndices;

    for (const auto& o : obj) {
        allVertexIndices.insert(allVertexIndices.end(), o.vertexIndices.begin(), o.vertexIndices.end());
        allTextureIndices.insert(allTextureIndices.end(), o.textureIndices.begin(), o.textureIndices.end());
        allNormalIndices.insert(allNormalIndices.end(), o.normalIndices.begin(), o.normalIndices.end());
    }

    return { coordinates, textures, normals, allVertexIndices, allTextureIndices, allNormalIndices };
}



