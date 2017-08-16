//
//  meshManager.cpp
//  meshSimplification
//
//  Created by MINGXIANG FAN on 8/13/17.
//
//

#include "meshManager.hpp"
#include <fstream>
#include <random>
#include <time.h>
#include <stack>
#include <map>
#include <cinder/gl/gl.h>

using namespace std;

mTriangle::mTriangle()
{
    this->mColor = cinder::Color(1,1,1);
    this->mNormal = cinder::vec3();
    for(size_t i = 0; i<3; i++)
    {
        this->sides[i] = 0;
        this->vertices[i] = 0;
    }
    this->groupNumber = 0;
}

mSides::mSides()
{
    this->mColor = this->mColor = cinder::Color(1,1,1);
    this->triangles[0] = 0;
    this->triangles[1] = 0;
    this->vertices[0] = 0;
    this->vertices[1] = 0;
    this->ridgeAngle = 0;
}

MeshEdge::MeshEdge()
{
    this->edgeAngle = 0;
    this->edgeColor = cinder::Color(1,1,1);
    this->edgeVertices[0] = NULL;
    this->edgeVertices[1] = NULL;
    this->adjacentTriangles[0] = NULL;
    this->adjacentTriangles[1] = NULL;
}

MeshTriangle::MeshTriangle()
{
    this->triangleColor = cinder::Color(1,1,1);
    this->triangleNormal = cinder::vec3(0,0,1);
    this->groupNumber = 0;
    this->triangleEdges[0] = NULL;
    this->triangleEdges[1] = NULL;
    this->triangleEdges[2] = NULL;
    this->triangleVertices[0] = NULL;
    this->triangleVertices[1] = NULL;
    this->triangleVertices[2] = NULL;
}

meshManager::meshManager()
{
    time_rec = time(0);
}

void meshManager::loadSTLFile(const char* filename)
{
    ifstream file(filename, ios::in | ios::binary);
    if (!file)
    {
        std::cerr << "file not exist" << std::endl;
        return;
    }
    char header_info[80] = "";
    char n_triangles[4];
    file.read(header_info, 80);
    file.read(n_triangles, 4);
    unsigned int * r = (unsigned int *)n_triangles;
    unsigned int triangle_num = *r;

    int vertex_count = 0;
    char n_buf[3 * sizeof(float)];
    char dummy[2];
    cinder::vec3 tmpPoint;
    std::vector<size_t> tmpTriangle;
    float * fptr;
    
    float xmax = -1E8, xmin=1E8, ymax=-1E8, ymin=1E8, zmax=-1E8, zmin=1E8;
    
    // use a*n*n + b * n + c for hash
    // assume max_vert < 1E4 && n_triangles < 1E6
    std::map<double, size_t> vertexMap;
    std::map<size_t, size_t> sideMap;
    
    for (unsigned int i = 0; i<triangle_num; i++) {
        
        //
        mTriangle tmpTriangle;
        //
        auto tmpMeshTriangle = std::make_shared<MeshTriangle>();
        //
        
        // read normal
        file.read(n_buf, 12);
        fptr = (float*) n_buf;
        
        //
        tmpTriangle.mNormal = (cinder::vec3(*fptr, *(fptr+1), *(fptr+2)));
        //
        tmpMeshTriangle->triangleNormal = cinder::vec3(*fptr, *(fptr+1), *(fptr+2));
        //
        
        // read vertices
        for (size_t ots = 0; ots<3; ots++)
        {
            file.read(n_buf, 12);
            fptr = (float*) n_buf;
            
            tmpPoint = cinder::vec3(*fptr, *(fptr+1), *(fptr+2));
            
            if (vertexMap.count(*fptr * 1E8 + *(fptr+1)*1E4 + *(fptr+2))>0)
            {
                //
                tmpTriangle.vertices[ots] = vertexMap[*fptr * 1E8 + *(fptr+1)*1E4 + *(fptr+2)];
                //
                tmpMeshTriangle->triangleVertices[ots] = _meshVertices[vertexMap[*fptr * 1E8 + *(fptr+1)*1E4 + *(fptr+2)]];
                //
            }
            else
            {
                xmax = max(xmax, *fptr);
                xmin = min(xmin, *fptr);
                ymax = max(ymax, *(fptr+1));
                ymin = min(ymin, *(fptr+1));
                zmax = max(zmax, *(fptr+2));
                zmin = min(zmin, *(fptr+2));
                vertexMap[*fptr * 1E8 + *(fptr+1)*1E4 + *(fptr+2)] = _points.size();
                
                //
                _points.push_back(tmpPoint);
                tmpTriangle.vertices[ots] = vertex_count;
                vertex_count++;
                //
                auto tmpVertex = std::make_shared<cinder::vec3>(*fptr, *(fptr+1), *(fptr+2));
                _meshVertices.push_back(tmpVertex);
                tmpMeshTriangle->triangleVertices[ots] = tmpVertex;
                //
            }
        }
        size_t idx1;
        size_t idx2;
        for(size_t k = 0; k<3; k++)
        {
            idx1 = k;
            idx2 = (k+1>=3)?(0):(k+1);
            if(sideMap.count(tmpTriangle.vertices[idx1] * 1000000 + tmpTriangle.vertices[idx2])>0)
            {
                auto tmpMeshEdge = _meshEdges[sideMap[tmpTriangle.vertices[idx1] * 1000000 + tmpTriangle.vertices[idx2]]];
                tmpMeshEdge->adjacentTriangles[1] = tmpMeshTriangle;
                tmpMeshTriangle->triangleEdges[k] = tmpMeshEdge;
                
                _sides[sideMap[tmpTriangle.vertices[idx1] * 1000000 + tmpTriangle.vertices[idx2]]].triangles[1] = _triangles.size();
                tmpTriangle.sides[k] = sideMap[tmpTriangle.vertices[idx1] * 1000000 + tmpTriangle.vertices[idx2]];
            }
            else if(sideMap.count(tmpTriangle.vertices[idx2] * 1000000 + tmpTriangle.vertices[idx1])>0)
            {
                auto tmpMeshEdge = _meshEdges[sideMap[tmpTriangle.vertices[idx2] * 1000000 + tmpTriangle.vertices[idx1]]];
                tmpMeshEdge->adjacentTriangles[1] = tmpMeshTriangle;
                tmpMeshTriangle->triangleEdges[k] = tmpMeshEdge;
                
                _sides[sideMap[tmpTriangle.vertices[idx2] * 1000000 + tmpTriangle.vertices[idx1]]].triangles[1] = _triangles.size();
                tmpTriangle.sides[k] = sideMap[tmpTriangle.vertices[idx2] * 1000000 + tmpTriangle.vertices[idx1]];
            }
            else
            {
                auto tmpMeshEdge = std::make_shared<MeshEdge>();
                tmpMeshEdge->edgeVertices[0] = tmpMeshTriangle->triangleVertices[idx1];
                tmpMeshEdge->edgeVertices[1] = tmpMeshTriangle->triangleVertices[idx2];
                tmpMeshEdge->adjacentTriangles[0] = tmpMeshTriangle;
                tmpMeshTriangle->triangleEdges[k] = tmpMeshEdge;
                _meshEdges.push_back(tmpMeshEdge);
                
                mSides tmpSide;
                tmpSide.vertices[0] = tmpTriangle.vertices[idx1];
                tmpSide.vertices[1] = tmpTriangle.vertices[idx2];
                tmpSide.triangles[0] =_triangles.size();
                sideMap[tmpTriangle.vertices[idx1] * 1E6 + tmpTriangle.vertices[idx2]] = _sides.size();
                tmpTriangle.sides[k] = _sides.size();
                _sides.push_back(tmpSide);
            }
        }
        _meshTriangles.push_back(tmpMeshTriangle);
        _triangles.push_back(tmpTriangle);
        file.read(dummy, 2);
    }
    float cx = (xmax+xmin)/2;
    float cy = (ymax+ymin)/2;
    float cz = (zmax+zmin)/2;
    for(size_t i = 0; i<_points.size(); i++)
    {
        _points[i][0] = (_points[i][0] - cx)*1000;
        _points[i][1] = (_points[i][1] - cy)*1000;
        _points[i][2] = (_points[i][2] - cz)*1000;
    }
    
    for(size_t i = 0; i<_meshVertices.size(); i++)
    {
        _meshVertices[i]->x = (_meshVertices[i]->x - cx)*1000;
        _meshVertices[i]->y = (_meshVertices[i]->y - cy)*1000;
        _meshVertices[i]->z = (_meshVertices[i]->z - cz)*1000;
    }
    
    std::cout<<"Done loading file at ";
    cout<<time(0) - time_rec<<"."<<endl;
    cout<<"Triagles: "<<_triangles.size()<<endl;
    cout<<"Sides: "<<_sides.size()<<endl;
    cout<<"Points: "<<_points.size()<<endl;
    cout<<"Triagles: "<<_meshTriangles.size()<<endl;
    cout<<"Sides: "<<_meshEdges.size()<<endl;
    cout<<"Points: "<<_meshVertices.size()<<endl;
}

void meshManager::writeSTLFile(const char* filename) {
    ofstream file(filename, ios::out | ios::binary | ios::trunc);
    if (!file)
    {
        std::cerr << "file not exist write" << std::endl;
        return;
    }
    char header_info[80] = "";
    size_t r = _triangles.size();
    cout << "triangle size: " << _triangles.size() << endl;
    char * n_triangles = (char*)&r;
    file.write(header_info, 80);
    file.write(n_triangles, 4);
    cout << "here" << endl;
    char dummy[2];
    
    for (unsigned int i = 0; i<_triangles.size(); i++) {
        float norm[3];
        norm[0] = _triangles[i].mNormal[0];
        norm[1] = _triangles[i].mNormal[1];
        norm[2] = _triangles[i].mNormal[2];
        // write normal
        file.write((char *)norm, 12);
        // write vertices
        float tmpFloat;
        for (size_t ots = 0; ots<3; ots++)
        {
            for (size_t j = 0; j<3; j++) {
                tmpFloat = _points[_triangles[i].vertices[ots]][j];
                file.write((char *)&tmpFloat, 4);
            }
        }
        file.write(dummy, 2);
    }
}

void meshManager::calculateRidgeAngles()
{
    cinder::vec3 n1, n2, p1, ridgeVect;
    cinder::vec3 normalCross;
    double zero_thresh = 1E-4;
    for(size_t i = 0; i<_sides.size(); i++)
    {
        n1 = cinder::normalize(_triangles[_sides[i].triangles[0]].mNormal);
        n2 = cinder::normalize(_triangles[_sides[i].triangles[1]].mNormal);
        ridgeVect = _points[_sides[i].vertices[1]] - _points[_sides[i].vertices[0]];
        size_t id1 = _triangles[_sides[i].triangles[0]].vertices[0] + _triangles[_sides[i].triangles[0]].vertices[1] + _triangles[_sides[i].triangles[0]].vertices[2] - _sides[i].vertices[1] - _sides[i].vertices[0];

        p1 = _points[id1] - _points[_sides[i].vertices[0]];
        p1 = p1 - cinder::dot(p1, ridgeVect) * ridgeVect;
        
        normalCross = cinder::cross(n1, n2);
        
        if( normalCross.x * normalCross.x + normalCross.y * normalCross.y + normalCross.z * normalCross.z < zero_thresh )
        {
            _sides[i].ridgeAngle = 0;
        }
        else
        {
            double cangle = cinder::dot(n1, n2);
            if(cinder::dot(p1, n2) > 0)
            {
                //concave
                _sides[i].ridgeAngle = -acos(cangle);
                _sides[i].mColor = cinder::Color(1,0,0);
            }
            else
            {
                //convex
                _sides[i].ridgeAngle = acos(cangle);
                _sides[i].mColor = cinder::Color(0,1,0);
            }
        }
    }
    std::cout<<"Done calculating angles at "<<std::endl;
    cout<<time(0) - time_rec<<endl;
}

void meshManager::drawFrame()
{
    /*
    cinder::gl::begin(GL_LINES);
    for(size_t i = 0; i<_sides.size(); i++)
    {
        cinder::gl::color(_sides[i].mColor);
        cinder::gl::vertex(_points[_sides[i].vertices[0]]);
        cinder::gl::vertex(_points[_sides[i].vertices[1]]);
    }
    cinder::gl::end();
     */
    
    cinder::gl::begin(GL_LINES);
    for(size_t i = 0; i<_meshEdges.size(); i++)
    {
        cinder::gl::color(_meshEdges[i]->edgeColor);
        cinder::gl::vertex(*(_meshEdges[i]->edgeVertices[0]));
        cinder::gl::vertex(*(_meshEdges[i]->edgeVertices[1]));
    }
    cinder::gl::end();
}

void meshManager::drawMeshes()
{
    cinder::gl::begin(GL_TRIANGLES);
    for(size_t i = 0; i<_triangles.size(); i++)
    {
        cinder::gl::color(_triangles[i].mColor);
        cinder::gl::vertex(_points[_triangles[i].vertices[0]]);
        cinder::gl::vertex(_points[_triangles[i].vertices[1]]);
        cinder::gl::vertex(_points[_triangles[i].vertices[2]]);
    }
    cinder::gl::end();

    /*
    std::default_random_engine generator(int(time(0)));
    std::uniform_real_distribution<double> distribution(0.3, 1);
    for(size_t i = 0; i<_triangles.size(); i++)
    {
        _meshTriangles[i]->triangleColor = cinder::Color(distribution(generator), distribution(generator), distribution(generator));
    }
    cinder::gl::begin(GL_TRIANGLES);
    for(size_t i = 0; i<_meshTriangles.size(); i++)
    {
        cinder::gl::color(_meshTriangles[i]->triangleColor);
        cinder::gl::vertex(*(_meshTriangles[i]->triangleVertices[0]));
        cinder::gl::vertex(*(_meshTriangles[i]->triangleVertices[1]));
        cinder::gl::vertex(*(_meshTriangles[i]->triangleVertices[2]));
    }
    cinder::gl::end();
     */
    
}

void meshManager::initTriMesh(cinder::TriMesh *tm)
{
    for(size_t i = 0; i<_points.size(); i++)
    {
        tm->appendPosition(_points[i]);
    }
    for(size_t i = 0; i<_triangles.size(); i++)
    {
        tm->appendTriangle(int(_triangles[i].vertices[0]), int(_triangles[i].vertices[1]), int(_triangles[i].vertices[2]));
    }
    tm->recalculateNormals();
}

void meshManager::watershedSegmentation(double segmentThreshold)
{
    size_t _meshGroupNum = 0;
    bool segmentationEnd = false;
    while(!segmentationEnd)
    {
        std::cout<<"segment iteration: "<<_meshGroupNum<<std::endl;
        _meshGroupNum++;
        std::stack<size_t> segStack;
        for(size_t i = 0; i<_triangles.size(); i++)
        {
            if(_triangles[i].groupNumber == 0)
            {
                segStack.push(i);
                break;
            }
        }
        while(!segStack.empty())
        {
            //std::cout<<"Stack: "<<segStack.size()<<std::endl;
            size_t currentMeshIndex = segStack.top();
            segStack.pop();
            _triangles[currentMeshIndex].groupNumber = _meshGroupNum;
            for(size_t i = 0; i<3; i++)
            {
                size_t nextIdx = _sides[_triangles[currentMeshIndex].sides[i]].triangles[0] + _sides[_triangles[currentMeshIndex].sides[i]].triangles[1] - currentMeshIndex;
                if( _triangles[nextIdx].groupNumber == 0 && abs(_sides[_triangles[currentMeshIndex].sides[i]].ridgeAngle) < segmentThreshold)
                {
                    segStack.push(nextIdx);
                }
            }
        }
        segmentationEnd = true;
        for(size_t i = 0; i<_triangles.size(); i++)
        {
            if(_triangles[i].groupNumber == 0)
            {
                segmentationEnd = false;
                segStack.push(i);
                break;
            }
        }
    }
    
    std::default_random_engine generator(int(time(0)));
    std::uniform_real_distribution<double> distribution(0.3, 1);
    
    vector<cinder::Color> tmpColors;
    cout<<"Segmentation results in "<<_meshGroupNum<<" parts. "<<endl;
    for(size_t i = 1; i<=_meshGroupNum; i++)
    {
        tmpColors.push_back(cinder::Color(distribution(generator), distribution(generator), distribution(generator)));
    }
    
    for(size_t j = 0; j<_triangles.size(); j++)
    {
        _triangles[j].mColor = tmpColors[_triangles[j].groupNumber - 1];
    }
    
    
    for(size_t i = 0; i<_sides.size(); i++)
    {
        _sides[i].mColor = cinder::Color(0,0,0);
        if( _triangles[_sides[i].triangles[0]].groupNumber != _triangles[_sides[i].triangles[1]].groupNumber )
        {
            //feature
            if(_sides[i].ridgeAngle > 0)
            {
                _sides[i].mColor = cinder::Color(1,0,0);
            }
            else
            {
                _sides[i].mColor = cinder::Color(0, 1,0);
            }
        }
    }
    cout<<"Done segmentation at "<<time(0) - time_rec<<endl;
}

void meshManager::refineMesh(size_t triangleIndex)
{
}

void meshManager::flipRidge(size_t ridgeIndex)
{
}

