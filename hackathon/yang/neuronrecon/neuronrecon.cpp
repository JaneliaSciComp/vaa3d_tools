// neuronrecon.cpp
// construct neuron tree(s) from detected signals

#include "neuronrecon.h"

#ifdef _MSC_VER
#define  LONG_LONG_MAX _I64_MAX

inline float  roundf(float num)  
{
    return num > 0 ? std::floor(num + 0.5f) : std::ceil(num - 0.5f);
}

inline long   lroundf(float num) { return static_cast<long>(roundf(num)); }   
#endif

#define getParent(n,nt) ((nt).listNeuron.at(n).pn<0)?(1000000000):((nt).hashNeuron.value((nt).listNeuron.at(n).pn))
#define dist(a,b) sqrt(((a).x-(b).x)*((a).x-(b).x)+((a).y-(b).y)*((a).y-(b).y)+((a).z-(b).z)*((a).z-(b).z))

template <class T> T pow2(T a)
{
    return a*a;
}

#ifndef MIN
#define MIN(a, b)  ( ((a)<(b))? (a) : (b) )
#endif
#ifndef MAX
#define MAX(a, b)  ( ((a)>(b))? (a) : (b) )
#endif

// class Plane
Plane::Plane()
{

}

Plane::~Plane()
{

}

// class Vector
Vector::Vector()
{

}

Vector::~Vector()
{

}

// axb
Vector* Vector::vcross(Vector *a, Vector *b)
{
    Vector *c = new Vector;
    c->dx = a->dy * b->dz - a->dz * b->dy;
    c->dy = a->dz * b->dx - a->dx * b->dz;
    c->dz = a->dx * b->dy - a->dy * b->dx;
    return (c);
}

// inner product
float Vector::vdot(Vector *a, Vector *b)
{
    return (a->dx * b->dx + a->dy * b->dy + a->dz * b->dz);
}

// magnitude
float Vector::vmag(Vector *a)
{
    return (sqrt(vdot(a, a)));
}

// reciprocal magnitude
float Vector::recip_vmag(Vector *a)
{
    return (1.0 / sqrt(vdot(a, a)));
}

// a /= ||a||
Vector* Vector::vnorm(Vector *a)
{
    float d;

    if ((d = recip_vmag(a)) > 1.0E6)
    {
        fprintf (stderr, "\nvector at 0x%x: %g %g %g?", a, a->dx, a->dy, a->dz);
        a->dx = a->dy = a->dz = 0.0;
        return (NULL);
    }
    else
    {
        a->dx *= d;
        a->dy *= d;
        a->dz *= d;
        return (a);
    }
}

void Vector::info()
{
    cout<<"vector: "<<dx<<" "<<dy<<" "<<dz<<endl;
}

// class Point
Point::Point()
{
    x = 0;
    y = 0;
    z = 0;
    radius = 0;
    val = 0;
    parents.clear();
    nn.clear();
    n=0;
    visited = false;
    connected = 0;
    pre = -1;
    next = -1;

    neighborVisited = false;
    isolated = false;
}

Point::Point (float x, float y, float z)
{
    this->x = x;
    this->y = y;
    this->z = z;

    radius = 0;
    val = 0;
    parents.clear();
    nn.clear();
    n=0;
    visited = false;

    connected = 0;
    pre = -1;
    next = -1;

    neighborVisited = false;
    isolated = false;
}

Point::~Point()
{
    parents.clear();
    children.clear();
    nn.clear();
}

void Point::setLocation(float x, float y, float z)
{
    this->x = x;
    this->y = y;
    this->z = z;
}

bool Point::isSamePoint(Point p)
{
    //
    if(abs(x - p.x)<1e-6 && abs(y - p.y)<1e-6 & abs(z - p.z)<1e-6)
    {
        return true;
    }

    //
    return false;
}

void Point::setRadius(float r)
{
    radius = r;
}

void Point::setValue(float v)
{
    val = v;
}

void Point::info()
{
    cout<<"Point: "<<n<<" ( "<<x<<" "<<y<<" "<<z<<" )"<<endl;
}

//
NCPointCloud::NCPointCloud()
{
    points.clear();
}

NCPointCloud::~NCPointCloud()
{
    points.clear();
}

int NCPointCloud::getPointCloud(QStringList files)
{
    //
    if(files.size()>0)
    {
        long n = files.size();

        for(long i=0; i<n; i++)
        {
            QString filename = files[i];

            if(filename.toUpper().endsWith(".SWC"))
            {
                NeuronTree nt = readSWC_file(filename);
                addPointFromNeuronTree(nt);
            }
        }
    }

    //
    return 0;
}

int NCPointCloud::addPointFromNeuronTree(NeuronTree nt)
{
    //
    if(nt.listNeuron.size()>0)
    {
        long n = nt.listNeuron.size();

        for(long i=0; i<n; i++)
        {
            Point p;

            p.n = nt.listNeuron[i].n;
            p.x = nt.listNeuron[i].x;
            p.y = nt.listNeuron[i].y;
            p.z = nt.listNeuron[i].z;
            p.radius = nt.listNeuron[i].r;
            p.parents.push_back(nt.listNeuron[i].parent);

            points.push_back(p);
        }

        // update children
        for(long i=0; i<n; i++)
        {
            if(points[i].parents[0]>0)
            {
                for(long j=0; j<n; j++)
                {
                    if(points[j].n == points[i].parents[0])
                    {
                        points[j].children.push_back(points[i].n);
                    }
                }
            }
        }

        // update type
        for(long i=0; i<n; i++)
        {
            if(points[i].parents[0] == -1)
            {
                // cell body
                points[i].type = -1;
            }
            else
            {
                if(points[i].children.size() > 0)
                {
                    if(points[i].children.size() > 1)
                    {
                        // branch points
                        points[i].type = 3;
                    }
                    else
                    {
                        // regular points
                        points[i].type = 1;
                    }
                }
                else
                {
                    // tip points
                    points[i].type = 0;
                }
            }
        }

    }

    //
    cout << "after add a neuron tree, the size of point cloud become "<<points.size()<<endl;

    //
    return 0;
}

int NCPointCloud::savePointCloud(QString filename)
{
    //
    long n = points.size();

    // .apo
    QList <CellAPO> pointcloud;

    for(long i=0; i<n; i++)
    {
        Point p = points[i];
        CellAPO cell;

        cell.x = p.x;
        cell.y = p.y;
        cell.z = p.z;
        cell.volsize = 2*p.radius;

        pointcloud.push_back(cell);
    }

    writeAPO_file(filename, pointcloud);

    //    QList<ImageMarker> markers;
    //    for(long i=0; i<n; i++)
    //    {
    //        Point p = points[i];
    //        ImageMarker marker(0, 1, p.x, p.y, p.z, p.radius);

    //        markers.push_back(marker);
    //    }

    //    //
    //    writeMarker_file(filename, markers);

    //
    return 0;
}

int NCPointCloud::saveNeuronTree(NCPointCloud pc, QString filename)
{
    // isolated points
    pc.isolatedPoints();

    //
    NeuronTree nt;

    for(long i=0; i<pc.points.size(); i++)
    {
        if(pc.points[i].isolated)
        {
            continue;
        }

        //
        NeuronSWC S;
        S.n = pc.points[i].n;
        S.type = 3;
        S.x = pc.points[i].x;
        S.y= pc.points[i].y;
        S.z = pc.points[i].z;
        S.r = pc.points[i].radius;
        S.pn = pc.points[i].parents[0];

        nt.listNeuron.append(S);
        nt.hashNeuron.insert(S.n, nt.listNeuron.size()-1);
    }

    //
    return writeESWC_file(filename, nt);
}

int NCPointCloud::resample()
{
    //

    //
    return 0;
}

float NCPointCloud::distance(Point a, Point b)
{
    float x = a.x - b.x;
    float y = a.y - b.y;
    float z = a.z - b.z;

    return sqrt(x*x + y*y + z*z);
}

int NCPointCloud::getBranchPoints(QString filename)
{
    //
    if(filename.toUpper().endsWith(".SWC"))
    {
        NeuronTree nt = readSWC_file(filename);
        addPointFromNeuronTree(nt);

        // find the branch point
        for(long i=0; i<points.size(); i++)
        {
            Point current;

            current.n = points[i].n;
            current.x = points[i].x;
            current.y = points[i].y;
            current.z = points[i].z;
            current.radius = points[i].radius;

            current.parents.push_back(points[i].parents[0]);

            // saving branches
            if(points[i].type == 3)
            {
                cout<<i<<" branch point: "<<points[i].n<< " - parent - "<<points[i].parents[0]<<endl;

                for(long j=0; j<points[i].children.size(); j++)
                {
                    cout<<" children "<<j<<" : "<<points[i].children[j]<<endl;
                }

                //
                NCPointCloud pc;

                Point parent, child1, child2;

                long pn, c1n, c2n;

                pn = points[i].parents[0];
                c1n = points[i].children[0];
                c2n = points[i].children[1];

                //
                for(long j=0; j<points.size(); j++)
                {
                    if(points[j].n == pn)
                    {
                        parent.n = points[j].n;
                        parent.x = points[j].x;
                        parent.y = points[j].y;
                        parent.z = points[j].z;
                        parent.radius = points[j].radius;

                        parent.parents.push_back(-1);

                        pn = points[i].parents[0];
                    }
                    else if(points[j].n == c1n)
                    {
                        child1.n = points[j].n;
                        child1.x = points[j].x;
                        child1.y = points[j].y;
                        child1.z = points[j].z;
                        child1.radius = points[j].radius;

                        child1.parents.push_back(points[i].n);
                    }
                    else if(points[j].n == c2n)
                    {
                        child2.n = points[j].n;
                        child2.x = points[j].x;
                        child2.y = points[j].y;
                        child2.z = points[j].z;
                        child2.radius = points[j].radius;

                        child2.parents.push_back(points[i].n);
                    }
                }

                pc.points.push_back(parent);
                pc.points.push_back(current);
                pc.points.push_back(child1);
                pc.points.push_back(child2);

                QString output = filename.left(filename.lastIndexOf(".")).append("_branch_%1.swc").arg(current.n);
                saveNeuronTree(pc, output);

                // recover parent's parent after saving the branch
                parent.parents[0] = pn;

                // segment start from the parent of branch point
                NCPointCloud pcSeg;
                long nseg = parent.n;
                bool keepFinding = true;

                if(parent.visited == false)
                {
                    //
                    pcSeg.points.push_back(parent);

                    //
                    while(keepFinding)
                    {
                        //
                        bool found = false;
                        for(long j=0; j<points.size(); j++)
                        {
                            if(points[j].n == parent.parents[0])
                            {
                                found = true;

                                //
                                if(points[j].visited == false && points[j].type == 1)
                                {
                                    parent.n = points[j].n;
                                    parent.x = points[j].x;
                                    parent.y = points[j].y;
                                    parent.z = points[j].z;
                                    parent.radius = points[j].radius;

                                    parent.parents[0] = points[j].parents[0];

                                    //
                                    pcSeg.points.push_back(parent);

                                    //
                                    break;
                                }
                                else
                                {
                                    keepFinding = false;
                                }
                            }
                        }

                        if(!found)
                        {
                            break;
                        }

                    }

                    //
                    if(pcSeg.points.size()>1)
                    {
                        QString outSegs = filename.left(filename.lastIndexOf(".")).append("_segment_%1.swc").arg(nseg);
                        saveNeuronTree(pcSeg, outSegs);
                    }
                }

            }
            else if(points[i].type == 0)
            {
                NCPointCloud pcSeg;
                long nseg = current.n;
                bool keepFinding = true;

                if(current.visited == false)
                {
                    //
                    pcSeg.points.push_back(current);

                    //
                    while(keepFinding)
                    {
                        //
                        bool found = false;
                        for(long j=0; j<points.size(); j++)
                        {
                            if(points[j].n == current.parents[0])
                            {
                                found = true;

                                //
                                if(points[j].visited == false && points[j].type == 1)
                                {
                                    current.n = points[j].n;
                                    current.x = points[j].x;
                                    current.y = points[j].y;
                                    current.z = points[j].z;
                                    current.radius = points[j].radius;

                                    current.parents[0] = points[j].parents[0];

                                    //
                                    pcSeg.points.push_back(current);

                                    //
                                    break;
                                }
                                else
                                {
                                    keepFinding = false;
                                }
                            }
                        }

                        if(!found)
                        {
                            break;
                        }

                    }

                    //
                    if(pcSeg.points.size()>1)
                    {
                        QString outSegs = filename.left(filename.lastIndexOf(".")).append("_segment_%1.swc").arg(nseg);
                        saveNeuronTree(pcSeg, outSegs);
                    }
                }
            } // type
        }
    }
    else
    {
        cout<<"please input a .swc file\n";
    }

    //
    return 0;
}

int NCPointCloud::getNeurites(QString filename)
{
    //


    //
    return 0;
}

float NCPointCloud::getAngle(Point a, Point b, Point c)
{
    // get angle between a->b and b->c

    float x1 = b.x - a.x;
    float y1 = b.y - a.y;
    float z1 = b.z - a.z;

    float x2 = c.x - b.x;
    float y2 = c.y - b.y;
    float z2 = c.z - b.z;

    float dot = x1*x2 + y1*y2 + z1*z2;
    float lenSq1 = x1*x1 + y1*y1 + z1*z1;
    float lenSq2 = x2*x2 + y2*y2 + z2*z2;

    //
    return acos(dot/sqrt(lenSq1 * lenSq2));
}

bool NCPointCloud::findNextUnvisitPoint(unsigned long &index)
{
    long n = points.size();

    if(n>0)
    {
        for(long i=0; i<n; i++)
        {
            if(points[i].visited==false)
            {
                index = i;
                return true;
            }
        }
    }

    return false;
}

int NCPointCloud::knn(int k, int radius)
{
    //
    if(k<1 || points.size()<k)
    {
        cout<<"invalid k or cloud\n";
        return -1;
    }

    //
    long n = points.size();
    PointCloud<float> cloud;
    cloud.pts.resize(n);

    for(long i=0; i<n; i++)
    {
        cloud.pts[i].x = points[i].x;
        cloud.pts[i].y = points[i].y;
        cloud.pts[i].z = points[i].z;
    }

    // construct a kd-tree index
    KDTreeSingleIndexAdaptor<L2_Simple_Adaptor<float, PointCloud<float> >, PointCloud<float>, 3> kdtree(3, cloud, KDTreeSingleIndexAdaptorParams(max(10, 2*k)));
    kdtree.buildIndex();

    //
    float *p = NULL;
    try
    {
        p = new float [3];
    }
    catch(...)
    {
        cout<<"fail to alloc memory for a point\n";
        return -1;
    }

    //
    for(long i=0; i<n; i++)
    {
        p[0] = cloud.pts[i].x;
        p[1] = cloud.pts[i].y;
        p[2] = cloud.pts[i].z;

        //cout<<"current point ... "<<i<<" "<<p[0]<<" "<<p[1]<<" "<<p[2]<<endl;

        // 0 is itself, so start recording nearest neighbor from 1
        if(radius)
        {
            //
            vector<pair<size_t,float> > ret_matches;
            nanoflann::SearchParams params;

            //
            const size_t nMatches = kdtree.radiusSearch(p, radius, ret_matches, params);

            //
            //cout << "radiusSearch(): radius=" << radius << " -> " << nMatches << " matches\n";
            for (size_t j = 1; j < ret_matches.size(); j++)
            {
                //cout << "idx["<< j << "]=" << ret_matches[j].first << " dist["<< j << "]=" << ret_matches[j].second << endl;
                points[i].nn.push_back(ret_matches[j].first);
            }
        }
        else
        {
            //
            vector<size_t> k_indices;
            k_indices.resize (k);
            vector<float> k_distances;
            k_distances.resize (k);

            //
            size_t num_results = kdtree.knnSearch(p, k, &k_indices[0], &k_distances[0]);

            //
            //cout << "knnSearch(): num_results=" << num_results << "\n";
            for (size_t j = 1; j < k_indices.size(); ++j)
            {
                //cout << "idx["<< j << "]=" << long(k_indices[j]) << " dist["<< j << "]=" << k_distances[j] << endl;
                points[i].nn.push_back(k_indices[j]);
            }
        }
    }

    //
    if(p)
    {
        delete []p;
        p = NULL;
    }

    //
    return 0;
}

int NCPointCloud::connectPoints2Lines(QString infile, QString outfile, int k, float angle, float m)
{
    // load point cloud save as a .apo file
    QList <CellAPO> inputPoints = readAPO_file(infile);

    long n = inputPoints.size();

    for(long i=0; i<n; i++)
    {
        CellAPO cell = inputPoints[i];

        //
        Point p;

        p.n = i+1; // # assigned
        p.x = cell.x;
        p.y = cell.y;
        p.z = cell.z;
        p.radius = 0.5*cell.volsize;

        points.push_back(p);
    }

    // find k nearest neighbors
    //    float mean, stddev;
    //    knnMeanStddevDist(mean, stddev, k);
    //    knn(k, mean);

    float searchingRadius;
    knnMaxDist(searchingRadius);
    knn(k, searchingRadius);

    // connect points into lines
    unsigned long loc;
    while(findNextUnvisitPoint(loc))
    {
        //
        //cout<<"current point ..."<<loc<<endl;

        //
        if(points[loc].visited == false)
        {
            points[loc].visited = true;
            points[loc].parents.push_back(-1);
        }

        //
        if(isConsidered(loc, m))
        {
            //
            Point p = points[loc];

            //cout<<" ... "<<p.x<<" "<<p.y<<" "<<p.z<<endl;

            //
            while(minAngle(loc, angle)>=0)
            {
                //cout<<"next point ... "<<loc<<endl;
            }
        }
    }

    cout<<"done connection ... \n";

    // save connected results into a .swc file

    if(!outfile.isEmpty())
    {
        saveNeuronTree(*this, outfile);
    }

    //
    return 0;
}

// cost func
int NCPointCloud::minAngle(unsigned long &loc, float maxAngle)
{
    //
    Point p = points[loc];

    //
    //unsigned int nneighbors = 5;
    //float maxAngle = 0.1; // 0.942; // threshold 60 degree (120 degree)
    float minAngle = maxAngle;

    long next = -1;
    long next_next = -1;

    //
    if(p.parents[0] == -1)
    {
        // connect points from -1 "soma" (starting point)
        for(long i=0; i<p.nn.size(); i++)
        {
            Point p_next = points[p.nn[i]];

            if(p_next.visited || p_next.isSamePoint(p))
            {
                continue;
            }

            //cout<<"next ... "<<p_next.x<<" "<<p_next.y<<" "<<p_next.z<<endl;

            for(long j=0; j<p_next.nn.size(); j++)
            {
                Point p_next_next = points[p_next.nn[j]];

                if(p_next_next.visited || p_next_next.isSamePoint(p_next) || p_next_next.isSamePoint(p))
                {
                    continue;
                }

                //cout<<"next next ... "<<p_next_next.x<<" "<<p_next_next.y<<" "<<p_next_next.z<<endl;

                float angle = getAngle(p, p_next, p_next_next);

                if(angle<minAngle)
                {
                    minAngle = angle;
                    next = p.nn[i];
                    next_next = p_next.nn[j];
                }
            }

        }

        //
        if(next>0 && next_next>0)
        {
            points[loc].connected++;
            points[next].visited = true;
            points[next].connected++;
            points[next].parents.push_back(p.n);
            points[next_next].visited = true;
            points[next_next].connected++;
            points[next_next].parents.push_back(points[next].n);

            points[next].pre = loc;
            points[next].next = next_next;

            points[next_next].pre = next;

            loc = next_next;

            //cout<<"update loc ... "<<loc<<" angle "<<minAngle<<endl;

            return 0;
        }
    }
    else
    {
        // connect points from the intermediate point
        Point p_pre = points[p.pre];

        for(long i=0; i<p.nn.size(); i++)
        {
            Point p_next = points[p.nn[i]];

            if(p_next.visited || p_next.isSamePoint(p) || p_next.isSamePoint(p_pre))
            {
                continue;
            }

            //cout<<"next ... "<<p_next.x<<" "<<p_next.y<<" "<<p_next.z<<endl;

            float angle = getAngle(p_pre, p, p_next);

            if(angle<maxAngle && angle<minAngle)
            {
                minAngle = angle;
                next = p.nn[i];
            }
        }

        if(next>0)
        {
            points[loc].connected++;
            points[next].visited = true;
            points[next].connected++;
            points[next].parents.push_back(p.n);

            points[next].pre = loc;

            loc = next;

            //cout<<"update loc ... "<<loc<<" angle "<<minAngle<<endl;

            return 0;
        }

    }

    //
    return -1;
}

int NCPointCloud::isolatedPoints()
{
    //
    for(std::vector<Point>::iterator it = points.begin(); it != points.end(); ++it)
    {
        if(it->parents[0]==-1)
        {
            bool deleted = true;
            for(std::vector<Point>::iterator j = points.begin(); j != points.end(); ++j)
            {
                if(j->n == it->n)
                    continue;

                if(j->parents[0] == it->n)
                {
                    deleted = false;
                    break;
                }
            }

            if(deleted)
            {
                it->isolated = true;
            }
        }
    }

    //
    return 0;
}

int NCPointCloud::resetVisitStatus()
{
    //
    for(std::vector<Point>::iterator it = points.begin(); it != points.end(); ++it)
    {
        it->visited = false;
    }

    //
    return 0;
}

//
float NCPointCloud::distPoint2LineSegment(Point a, Point b, Point p)
{
    //
    Point w, v, pb;

    w.x = p.x - a.x;
    w.y = p.y - a.y;
    w.z = p.z - a.z;

    v.x = b.x - a.x;
    v.y = b.y - a.y;
    v.z = b.z - a.z;

    float dot = w.x*v.x + w.y*v.y + w.z*v.z;

    if(dot <=0 )
    {
        return distance(a, p);
    }

    float dot2 = v.x*v.x + v.y*v.y + v.z*v.z;

    if(dot2 <= dot)
    {
        return distance(b, p);
    }

    float k = dot / dot2;

    pb.x = a.x + k*v.x;
    pb.y = a.y + k*v.y;
    pb.z = a.z + k*v.z;

    //
    return distance(p, pb);
}

//
bool NCPointCloud::isConsidered(unsigned long &index, float m)
{
    // assume knn has been run
    Point p = points[index];

    float thresh = m*p.radius;

    if(p.nn.size()<1)
    {
        cout<<"point #"<<p.n<<" without nearest neighbor ... increase your searching radius to include this point\n";
        return false;
    }
    else
    {
        for(long i=0; i<p.nn.size(); i++)
        {
            Point p1 = points[p.nn[i]];

            if(p1.pre!=-1)
            {
                Point p1_pre = points[p1.pre];

                if(distPoint2LineSegment(p1_pre, p1, p) <= thresh)
                {
                    points[index].visited = true;
                    points[index].parents.push_back(-1);
                    return false;
                }

                if(p1.next!=-1)
                {
                    Point p1_next = points[p1.next];

                    if(distPoint2LineSegment(p1, p1_next, p) <= thresh)
                    {
                        points[index].visited = true;
                        points[index].parents.push_back(-1);
                        return false;
                    }
                }
            }
            else
            {
                if(p1.next!=-1)
                {
                    Point p1_next = points[p1.next];

                    if(distPoint2LineSegment(p1, p1_next, p) <= thresh)
                    {
                        points[index].visited = true;
                        points[index].parents.push_back(-1);
                        return false;
                    }
                }
            }
        }
    }

    //
    return true;
}

int NCPointCloud::delDuplicatedPoints()
{
    // tmp points
    std::vector<Point> pc;
    pc.clear();

    //
    for(std::vector<Point>::iterator it = points.begin(); it != points.end(); ++it)
    {
        if(pc.size()<1)
        {
            pc.push_back(*it);
        }
        else
        {
            bool found = false;
            for(std::vector<Point>::iterator i = pc.begin(); i != pc.end(); ++i)
            {
                if(i->isSamePoint(*it))
                {
                    found = true;
                    continue;
                }
            }

            if(found==false)
            {
                pc.push_back(*it);
            }
        }
    }

    // copy back
    points.clear();

    for(std::vector<Point>::iterator it = pc.begin(); it != pc.end(); ++it)
    {
        points.push_back(*it);
    }

    //
    return 0;
}

int NCPointCloud::copy(NCPointCloud pc)
{
    //
    if(pc.points.size()<1)
    {
        cout<<"Empty input\n";
        return -1;
    }

    //
    points.clear();

    for(std::vector<Point>::iterator it = pc.points.begin(); it != pc.points.end(); ++it)
    {
        points.push_back(*it);
    }

    //
    return 0;
}

// breadth-first sort
int NCPointCloud::ksort(NCPointCloud pc, int k)
{
    //
    points.clear();

    //
    pc.knn(k);

    //
    float maxradius = 0;
    long soma = 0;
    // find the point with biggest radius
    for(long i=0; i<pc.points.size(); i++)
    {
        if(pc.points[i].radius > maxradius)
        {
            maxradius = pc.points[i].radius;
            soma = i;
        }
    }

    // sort
    while(points.size() < pc.points.size())
    {
        if(points.size()<1)
        {
            // soma
            points.push_back(pc.points[soma]);
            //points[0].visited = true;
            points[0].n = soma;
            pc.points[soma].visited = true;
        }
        else
        {
            // add neighbors
            bool allvisited = true;
            long n = points.size();
            for(long i=0; i<n; i++)
            {
                int cur = points[i].n;
                if(pc.points[cur].neighborVisited)
                {
                    continue;
                }

                //
                pc.points[cur].neighborVisited = true;

                //
                for(int j=1; j<pc.points[cur].nn.size(); j++)
                {
                    int neighbor = pc.points[cur].nn[j];
                    if(!pc.points[neighbor].visited)
                    {
                        allvisited = false;

                        pc.points[neighbor].visited = true;
                        points.push_back(pc.points[neighbor]);
                        points[points.size()-1].n = neighbor;
                    }
                }
            }

            if(allvisited)
            {
                // find unvisited one from input point cloud
                unsigned long loc;
                pc.findNextUnvisitPoint(loc);

                pc.points[loc].visited = true;
                points.push_back(pc.points[loc]);
                points[points.size()-1].n = loc;
            }
        }
    }

    //
    return 0;
}

int NCPointCloud::knnMeanStddevDist(float &mean, float &stddev, int k)
{
    //
    NCPointCloud pc;
    pc.copy(*this);

    //
    pc.knn(k);

    //
    mean = 0;
    stddev = 0;

    // mean
    long n = 0;

    for(long i=0; i<pc.points.size(); i++)
    {
        Point p = pc.points[i];

        for(long j=1; j<pc.points[i].nn.size(); j++)
        {
            Point q = pc.points[pc.points[i].nn[j]];

            if(q.isSamePoint(p))
            {
                continue;
            }
            else
            {
                mean += distance(p,q);
                n++;
            }
        }
    }

    cout<<k<<" nn ... "<<mean<<" / "<<n<<endl;

    if(n>0)
    {
        mean /= n;
    }

    cout<<"mean distance: "<<mean<<endl;

    // stdard deviation


    //
    return 0;
}

int NCPointCloud::knnMaxDist(float &max)
{
    //
    NCPointCloud pc;
    pc.copy(*this);

    // 1 nearest neighbor
    pc.knn(2);

    //
    max = 0;
    for(long i=0; i<pc.points.size(); i++)
    {
        Point p = pc.points[i];

        for(long j=0; j<pc.points[i].nn.size(); j++)
        {
            Point q = pc.points[pc.points[i].nn[j]];

            if(q.isSamePoint(p))
            {
                continue;
            }
            else
            {
                float dist = distance(p,q);

                if(dist > max)
                    max = dist;
            }
        }
    }

    //
    cout<<"max ... "<<max<<endl;

    //
    return 0;
}

Point NCPointCloud::pplusv(Point *p, Vector *v)
{
    Point a;
    a.x = p->x + v->dx;
    a.y = p->y + v->dy;
    a.z = p->z + v->dz;
    return (a);
}

float NCPointCloud::point_plane_dist(Point *a, Plane *P)
{
    return (a->x * P->a + a->y * P->b + a->z * P->c + P->d);
}

Point NCPointCloud::plerp(Point *a, Point *b, float t)
{
    Point p;
    p.x = t * b->x + (1-t) * a->x;
    p.y = t * b->y + (1-t) * a->y;
    p.z = t * b->z + (1-t) * a->z;
    return (p);
}

Point NCPointCloud::intersect_line_plane(Point *a, Point *b, Plane *M)
{
    Point p;
    float Mdota, Mdotb, denom, t;

    Mdota = point_plane_dist (a, M);
    Mdotb = point_plane_dist (b, M);

    denom = Mdota - Mdotb;
    if (fabsf(denom / (fabsf(Mdota) + fabsf(Mdotb))) < 1E-6) {
        fprintf (stderr, "int_line_plane(): no intersection?\n");
        p.x = p.y = p.z = 0.0;
    }
    else    {
        t = Mdota / denom;
        p = plerp (a, b, t);
    }

    return (p);
}

Point NCPointCloud::intersect_dline_plane(Point *a, Vector *adir, Plane *M)
{
    Point b = pplusv (a, adir);
    return (intersect_line_plane (a, &b, M));
}

Plane* NCPointCloud::plane_from_two_vectors_and_point(Vector *u, Vector *v, Point *p)
{
    Plane *M = new Plane;
    Vector *t = t->vcross (u, v);
    t = t->vnorm(t);

    M->a = t->dx;
    M->b = t->dy;
    M->c = t->dz;

    // plane contains p
    M->d = -(M->a * p->x + M->b * p->y + M->c * p->z);
    return (M);
}

int NCPointCloud::line_line_closest_point(Point &pA, Point &pB, Point *a, Vector *adir, Point *b, Vector *bdir)
{
    Vector *cdir;
    Plane *ac, *bc;

    // connecting line is perpendicular to both
    cdir = cdir->vcross(adir, bdir);

    // lines are near-parallel -- all points are closest
    if (!cdir->vnorm(cdir))   {
        pA = *a;
        pB = *b;
        return (0);
    }

    // form plane containing line A, parallel to cdir
    ac = plane_from_two_vectors_and_point(cdir, adir, a);

    // form plane containing line B, parallel to cdir
    bc = plane_from_two_vectors_and_point(cdir, bdir, b);

    // closest point on A is line A ^ bc
    pA = intersect_dline_plane(a, adir, bc);

    // closest point on B is line B ^ ac
    pB = intersect_dline_plane(b, bdir, ac);

    if (distance(pA, pB) < 1E-6)
        return (1);     // coincident
    else
        return (2);     // distinct
}

int NCPointCloud::mergeLines(float maxAngle)
{
    // connect a line to all other lines by evaluating
    // 1. distance
    // 2. intersections and angles

    // merge lines
    bool merging = true;
    long iter = 0;
    size_t top = 3;
    while(merging)
    {
        merging = false;

        // update lines after each merging
        vector<LineSegment> lines;
        resetVisitStatus();

        //
        for(long i=0; i<points.size(); i++)
        {
            if(points[i].parents[0]==-1)
            {
                if(points[i].children.size()>0 && points[i].visited==false)
                {
                    LineSegment ls;

                    long j=i;
                    while(points[j].children.size()>0)
                    {
                        ls.points.push_back(points[j]);
                        points[j].visited = true;

                        j = indexofpoint(points[j].children[0]);

                        if(j<0)
                            break;
                    }
                    ls.points.push_back(points[j]); // end point


                    if(ls.points.size()>2)
                    {
                        ls.boundingbox();
                        ls.lineFromPoints();
                        lines.push_back(ls);
                    }
                }
            }
        }

        cout<<"iter #"<<++iter<<" : "<<lines.size()<<" lines to be considered merging"<<endl;

        // test
        for(int i=0; i<lines.size(); i++)
        {
            LineSegment line = lines[i];

            cout<<"test ... line "<<i<<" "<<line.points.size()<<endl;
            for(int k=0; k<line.points.size(); k++)
            {
                cout<<"test line point's parent ? "<<k<<" "<<line.points[k].n<<" "<<line.points[k].parents.size()<<endl;
                cout<<"... parent "<<line.points[k].parents[0]<<endl;
            }
        }


        if(lines.size()>1)
        {
            //
            Point pi, pj;
            LineSegment lsi, lsj;
            for(long i=0; i<lines.size(); i++)
            {
                cout<<"line i:"<<i<<endl;

                lsi = lines[i];

                lines[i].visited = true;
                vector<tuple<float,float,long>> candidates;
                vector<pair<Point,long>> jPoints;

                for(long j=1; j<lines.size(); j++)
                {
                    if(j==i)
                        continue;

                    cout<<"line j:"<<j<<endl;

                    lsj = lines[j];

                    Point pA, pB;
                    line_line_closest_point(pA, pB, lsi.origin, lsi.axis, lsj.origin, lsj.axis);

                    cout<<"evaluating line "<<i<<" & "<<j<<endl;
                    lsi.origin->info();
                    lsi.axis->info();

                    lsj.origin->info();
                    lsj.axis->info();

                    if(maxAngle>1 && (lsi.insideLineSegments(pA) || lsj.insideLineSegments(pB)))
                    {
                        cout<<lsi.insideLineSegments(pA)<<endl;
                        pA.info();
                        cout<<lsj.insideLineSegments(pB)<<endl;
                        pB.info();

                        continue;
                    }
                    else
                    {
                        // angle
                        Point t1 = lsi.points[0];
                        Point t2 = lsi.points[lsi.points.size()-1];

                        if(distance(t1, pA) < distance(t2, pA))
                        {
                            pi = t1;
                        }
                        else
                        {
                            pi = t2;
                        }

                        Point q1 = lsj.points[0];
                        Point q2 = lsj.points[lsj.points.size()-1];

                        if(distance(q1, pB) < distance(q2, pB))
                        {
                            pj = q1;
                        }
                        else
                        {
                            pj = q2;
                        }

                        float angle = getAngle(pi, pA, pj);
                        candidates.push_back(make_tuple(distance(pi, pj), angle, j));
                        jPoints.push_back(make_pair(pj,j));

                        cout<<"angle "<<angle<<" < "<<maxAngle<<endl;

                        if(angle>1.57)
                            angle = 3.14 - angle;

                        if(angle<maxAngle)
                            merging = true;
                    }
                } // j

                if(merging)
                {
                    // sort by angle
                    sort(candidates.begin(), candidates.end(), [](const tuple<float,float,long>& a, const tuple<float,float,long>& b) -> bool
                    {
                        return std::get<1>(a) < std::get<1>(b);
                    });

                    // sort by distance
                    if(candidates.size()>top)
                    {
                        sort(candidates.begin(), candidates.begin()+top, [](const tuple<float,float,long>& a, const tuple<float,float,long>& b) -> bool
                        {
                            return std::get<0>(a) < std::get<0>(b);
                        });
                    }
                    else
                    {
                        sort(candidates.begin(), candidates.end(), [](const tuple<float,float,long>& a, const tuple<float,float,long>& b) -> bool
                        {
                            return std::get<0>(a) < std::get<0>(b);
                        });
                    }

                    // merge
                    long tipi = indexofpoint(pi.n);
                    long tipj;
                    long jLine = get<2>(candidates[0]);
                    for(long k=0; k<jPoints.size(); k++)
                    {
                        if(jPoints[k].second == jLine)
                        {
                            tipj = indexofpoint(jPoints[k].first.n);
                            break;
                        }
                    }

                    //
                    cout<<"connecting ... line #"<<i<<" to line #"<<jLine<<endl;
                    cout<<"connected tip point #"<<tipi<<" tip point #"<<tipj<<endl;

                    cout<<"checking ... "<<points[tipi].parents[0]<<" "<<points[tipj].parents[0]<<endl;

                    //
                    if(pi.parents[0]==-1 && pj.parents[0]!=-1)
                    {
                        points[tipi].parents[0] = points[tipj].n;
                        points[tipj].children.push_back(points[tipi].n);
                    }
                    else if(pi.parents[0]!=-1 && pj.parents[0]==-1)
                    {
                        points[tipj].parents[0] = points[tipi].n;
                        points[tipi].children.push_back(points[tipj].n);
                    }
                    else
                    {
                        // inverse the short line and connect it to the other
                        if(lsi.points.size() < lsj.points.size())
                        {
                            cout<<"inverse lsi \n";

                            // inverse lsi
                            long idxcur, ncur, nnext;
                            long n = lsi.points.size() - 1;

                            idxcur = indexofpoint(lsi.points[n].n);
                            ncur = lsi.points[n].n;
                            nnext = points[idxcur].parents[0];
                            points[idxcur].parents[0] = -1;
                            while(n>0)
                            {
                                n--;

                                idxcur = indexofpoint(nnext);
                                nnext = points[idxcur].parents[0];
                                points[idxcur].parents[0] = ncur;
                                ncur = nnext;
                            }

                            if(points[tipi].parents[0]==-1)
                            {
                                // pj -> pi
                                points[tipj].parents[0] = points[tipi].n;
                                points[tipi].children.push_back(points[tipj].n);
                            }
                            else
                            {
                                points[tipi].parents[0] = points[tipj].n;
                                points[tipj].children.push_back(points[tipi].n);
                            }
                        }
                        else
                        {
                            cout<<"inverse lsj "<<lsj.points.size()<<endl;

                            // inverse lsj
                            long idxcur, ncur, nnext;
                            long n = lsj.points.size() - 1;

                            idxcur = indexofpoint(lsj.points[n].n);
                            ncur = lsj.points[n].n;

                            cout<<"access idxcur and its parent "<<idxcur<<" "<<points.size()<<endl;

                            for(int k=0; k<=n; k++)
                                cout<<"test lsj point's parent "<<k<<" "<<lsj.points[k].n<<" "<<lsj.points[k].parents[0]<<endl;

                            nnext = points[idxcur].parents[0];
                            points[idxcur].parents[0] = -1;
                            while(n>0)
                            {
                                n--;

                                cout<<"nnext "<<nnext<<endl;

                                idxcur = indexofpoint(nnext);
                                nnext = points[idxcur].parents[0];

                                cout<<" ... nnext "<<nnext<<endl;

                                points[idxcur].parents[0] = ncur;
                                ncur = nnext;
                            }

                            if(points[tipj].parents[0]==-1)
                            {
                                // pi -> pj
                                points[tipi].parents[0] = points[tipj].n;
                                points[tipj].children.push_back(points[tipi].n);
                            }
                            else
                            {
                                points[tipj].parents[0] = points[tipi].n;
                                points[tipi].children.push_back(points[tipj].n);
                            }
                        }
                    }

                    // update lines
                    break;
                }
            } // i
        }
    } // while

    //
    return 0;


    /*
    // learning the rule/costfunc


    //
    for(int i=0; i<lines.size(); i++)
    {
        LineSegment lsi = lines[i];

        lines[i].visited = true;
        vector<pair<float,float> > comparelines;

        for(int j=1; j<lines.size(); j++)
        {
            if(j==i)
                continue;

            LineSegment lsj = lines[j];

            Point pA, pB;
            line_line_closest_point(pA, pB, lsi.origin, lsi.axis, lsj.origin, lsj.axis);

            if(lsi.insideLineSegments(pA) || lsj.insideLineSegments(pB))
            {
                continue;
            }
            else
            {
                cout<<"point A: \n";
                pA.info();
                cout<<"point B: \n";
                pB.info();

                cout<<"line i "<<i<<endl;
                lsi.origin->info();
                lsi.axis->info();

                lsi.points.begin()->info();
                lsi.points[lsi.points.size()-1].info();

                cout<<"line j "<<j<<endl;
                lsj.origin->info();
                lsj.axis->info();

                lsj.points.begin()->info();
                lsj.points[lsj.points.size()-1].info();

                // angle
                Point pi, pj;
                Point t1 = lsi.points[0];
                Point t2 = lsi.points[lsi.points.size()-1];

                if(distance(t1, pA) < distance(t2, pA))
                {
                    pi = t1;
                }
                else
                {
                    pi = t2;
                }

                Point q1 = lsj.points[0];
                Point q2 = lsj.points[lsj.points.size()-1];

                if(distance(q1, pB) < distance(q2, pB))
                {
                    pj = q1;
                }
                else
                {
                    pj = q2;
                }

                cout<<"point i "<<pi.x<<" "<<pi.y<<" "<<pi.z<<endl;
                cout<<"point j "<<pj.x<<" "<<pj.y<<" "<<pj.z<<endl;

                cout<<"distance of the 2 lines: "<<distance(pi, pj)<<endl;

                float angle = getAngle(pi, pA, pj);

                cout<<"angle ... "<<(1 - angle/3.14159)*180<<endl;

                comparelines.push_back(std::make_pair(distance(pi, pj), (1 - angle/3.14159)*180));


                if((1 - angle/3.14159)*180 > 160)
                    cout<<"j line #"<<j<<endl;
            }
        } // j

        for(int k=0; k<comparelines.size(); k++)
        {
            cout<<"line #"<<i<<" "<<comparelines[k].first<<" "<<comparelines[k].second<<endl;
        }

    } // i

    //
    cout<<endl;

    //
    int j=0;
    for(int i=0; i<lines[j].points.size(); i++)
    {
        cout<<lines[j].points[i].n<<" "<<3<<" "<<lines[j].points[i].x<<" "<<lines[j].points[i].y<<" "<<lines[j].points[i].z<<" "<<5<<" "<<lines[j].points[i].parents[0]<<endl;
    }

    // j 4
    j=4;
    for(int i=0; i<lines[j].points.size(); i++)
    {
        cout<<lines[j].points[i].n<<" "<<3<<" "<<lines[j].points[i].x<<" "<<lines[j].points[i].y<<" "<<lines[j].points[i].z<<" "<<5<<" "<<lines[j].points[i].parents[0]<<endl;
    }

    // j 7
    j=7;
    for(int i=0; i<lines[j].points.size(); i++)
    {
        cout<<lines[j].points[i].n<<" "<<3<<" "<<lines[j].points[i].x<<" "<<lines[j].points[i].y<<" "<<lines[j].points[i].z<<" "<<5<<" "<<lines[j].points[i].parents[0]<<endl;
    }
    //
    return 0;

    // first try
    //
    for(int i=0; i<lines.size(); i++)
    {
        LineSegment lsi = lines[i];

        lines[i].visited = true;

        for(int j=1; j<lines.size(); j++)
        {
            if(j==i)
                continue;

            LineSegment lsj = lines[j];

            if(lsj.visited)
            {
                continue;
            }

            Point pA, pB;
            line_line_closest_point(pA, pB, lsi.origin, lsi.axis, lsj.origin, lsj.axis);

            // connect them

            if(lsi.insideLineSegments(pA) || lsj.insideLineSegments(pB))
            {
                continue;
            }
            else
            {
                cout<<"checking angles ... "<<lsi.points.size()<<" "<<lsj.points.size()<<endl;

                // angle
                Point pi, pj;
                Point t1 = lsi.points[0];
                Point t2 = lsi.points[lsi.points.size()-1];

                if(distance(t1, pA) < distance(t2, pA))
                {
                    pi = t1;
                }
                else
                {
                    pi = t2;
                }

                Point q1 = lsj.points[0];
                Point q2 = lsj.points[lsj.points.size()-1];

                if(distance(q1, pB) < distance(q2, pB))
                {
                    pj = q1;
                }
                else
                {
                    pj = q2;
                }

                cout<<"point i "<<pi.x<<" "<<pi.y<<" "<<pi.z<<endl;
                cout<<"point j "<<pj.x<<" "<<pj.y<<" "<<pj.z<<endl;

                float angle = getAngle(pi, pA, pj);

                cout<<"angle ... "<<angle<<endl;

                // connect them
                if(angle < maxAngle)
                {
                    long i = indexofpoint(pi.n);
                    long j = indexofpoint(pj.n);

                    if(pi.parents[0]==-1 && pj.parents[0]!=-1)
                    {
                        points[i].parents[0] = points[j].n;
                    }
                    else if(pi.parents[0]!=-1 && pj.parents[0]==-1)
                    {
                        points[j].parents[0] = points[i].n;
                    }
                    else
                    {
                        // inverse the short line and connect it to the other
                        if(lsi.points.size() < lsj.points.size())
                        {
                            // inverse lsi
                            long idxcur, ncur;
                            long n = lsi.points.size() - 1;
                            for(long k = n; k>0; --k)
                            {
                                idxcur = indexofpoint(lsi.points[k].n);

                                cout<<"1 idxcur ... "<<idxcur<<endl;

                                if(k == n)
                                {
                                    points[idxcur].parents[0] = -1;
                                }
                                else
                                {
                                    points[idxcur].parents[0] = ncur;
                                }

                                ncur = points[idxcur].n;
                            }

                            if(pi.parents[0]==-1)
                            {
                                // pj -> pi
                                points[j].parents[0] = points[i].n;
                            }
                            else
                            {
                                points[i].parents[0] = points[j].n;
                            }
                        }
                        else
                        {
                            // inverse lsj
                            long idxcur, ncur;
                            long n = lsj.points.size() - 1;
                            for(long k = n; k>0; --k)
                            {
                                idxcur = indexofpoint(lsj.points[k].n);

                                cout<<"2 idxcur ... "<<idxcur<<" "<<lsj.points[k].n<<endl;

                                if(k == n)
                                {
                                    points[idxcur].parents[0] = -1;
                                }
                                else
                                {
                                    points[idxcur].parents[0] = ncur;
                                }

                                ncur = points[idxcur].n;
                            }

                            if(pj.parents[0]==-1)
                            {
                                // pi -> pj
                                points[i].parents[0] = points[j].n;
                            }
                            else
                            {
                                points[j].parents[0] = points[i].n;
                            }
                        }
                    }
                }

            }

        }
    }

    //
    return 0;
    */
}

long NCPointCloud::indexofpoint(long n)
{
    for(long i=0; i<points.size(); i++)
    {
        if(points[i].n == n)
            return i;
    }

    return -1;
}

// class Quadruple
Quadruple::Quadruple()
{

}

Quadruple::~Quadruple()
{

}

int Quadruple::find3nearestpoints(Point p, NCPointCloud pc)
{

}

// class LineSegment
LineSegment::LineSegment()
{
    meanval_adjangles = 0;
    stddev_adjangles = 0;
    points.clear();
    b_bbox = false;
    visited = false;
}

LineSegment::~LineSegment()
{
    meanval_adjangles = 0;
    stddev_adjangles = 0;
    points.clear();
    b_bbox = false;
    visited = false;
}

int LineSegment::getMeanDev()
{
    // mean and stdard deviation
    if(points.size()<3)
    {
        cout<<"Not enough points for statistics\n";
        return -1;
    }
    else
    {
        //
        Point firstPoint = points.back();
        points.pop_back();

        Point secondPoint = points.back();
        points.pop_back();

        long n = 0;
        float sum = 0;

        vector<float> angles;

        while(!points.empty())
        {
            Point thirdPoint = points.back();
            points.pop_back();

            if(thirdPoint.x == secondPoint.x && thirdPoint.y == secondPoint.y && thirdPoint.z == secondPoint.z)
            {
                // duplicated point
                continue;
            }

            float angle = 3.14159265f - getAngle(firstPoint, secondPoint, thirdPoint);

            sum += angle;
            angles.push_back(angle);

            n++;

            qDebug()<<"angle ... "<<angle<<" sum "<<sum;

            firstPoint = secondPoint;
            secondPoint = thirdPoint;

            qDebug()<<"first point ..."<<firstPoint.x<<firstPoint.y<<firstPoint.z;
        }

        qDebug()<<"sum of "<<n<<" angles ... "<<sum;

        //
        meanval_adjangles = sum / n;

        //
        sum = 0;
        for(int i=0; i<angles.size(); i++)
        {
            sum += (angles[i] - meanval_adjangles)*(angles[i] - meanval_adjangles);
        }

        stddev_adjangles = sqrt(sum/(n-1));

        qDebug()<<"mean ... "<<meanval_adjangles<<" std dev ..."<<stddev_adjangles;
    }

    //
    return 0;
}

int LineSegment::save(QString filename)
{
    //
    std::ofstream outfile;

    outfile.open(filename.toStdString().c_str(), std::ios_base::app);
    outfile << meanval_adjangles << ", " << stddev_adjangles << endl;

    //
    return 0;
}

int LineSegment::boundingbox()
{
    if(points.size()>0)
    {
        for(long i=0; i<points.size(); i++)
        {
            Point p = points[i];

            if(i==0)
            {
                pbbs.x = pbbe.x = p.x;
                pbbs.y = pbbe.y = p.y;
                pbbs.z = pbbe.z = p.z;
            }
            else
            {
                if(p.x<pbbs.x)
                    pbbs.x = p.x;
                if(p.x>pbbe.x)
                    pbbe.x = p.x;

                if(p.y<pbbs.y)
                    pbbs.y = p.y;
                if(p.y>pbbe.y)
                    pbbe.y = p.y;

                if(p.z<pbbs.z)
                    pbbs.z = p.z;
                if(p.z>pbbe.z)
                    pbbe.z = p.z;
            }
        }
    }
    else
    {
        return -1;
    }

    b_bbox = true;

    //
    return 0;
}

bool LineSegment::insideLineSegments(Point p)
{
    if(b_bbox == false)
        boundingbox();

    if(p.x >= pbbs.x && p.x <= pbbe.x && p.y >= pbbs.y && p.y <= pbbe.y && p.z >= pbbs.z && p.z <= pbbe.z)
        return true;

    return false;
}

int LineSegment::lineFromPoints()
{
    auto start = std::chrono::high_resolution_clock::now();

    // copy coordinates to  matrix in Eigen format
    size_t num_atoms = points.size();
    Eigen::Matrix< Eigen::Vector3d::Scalar, Eigen::Dynamic, Eigen::Dynamic > centers(num_atoms, 3);
    for (size_t i = 0; i < num_atoms; ++i)
    {
        centers.row(i) = Eigen::Vector3d(points[i].x, points[i].y, points[i].z);
    }

    Eigen::Vector3d v_origin = centers.colwise().mean();

    origin = new Point;

    origin->x = v_origin(0);
    origin->y = v_origin(1);
    origin->z = v_origin(2);

    Eigen::MatrixXd centered = centers.rowwise() - v_origin.transpose();
    Eigen::MatrixXd cov = centered.adjoint() * centered;
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> eig(cov);
    Eigen::Vector3d v_axis = eig.eigenvectors().col(2).normalized();

    axis = new Vector;

    axis->dx = v_axis(0);
    axis->dy = v_axis(1);
    axis->dz = v_axis(2);

    auto end = std::chrono::high_resolution_clock::now();

    //cout<<"estimate a line in "<<std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()<<endl;

    //
    return 0;
}



