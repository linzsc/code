#include <QApplication>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QMouseEvent>
#include <QLabel>
#include <gdal.h>
#include <gdal_priv.h>
#include <cmath>
#include <vector>

// Haversine formula to calculate distance between two geographic coordinates
double haversine(double lon1, double lat1, double lon2, double lat2) {
    const double R = 6371e3; // Earth radius in meters
    double phi1 = lat1 * M_PI / 180.0;
    double phi2 = lat2 * M_PI / 180.0;
    double deltaPhi = (lat2 - lat1) * M_PI / 180.0;
    double deltaLambda = (lon2 - lon1) * M_PI / 180.0;

    double a = sin(deltaPhi / 2) * sin(deltaPhi / 2) +
               cos(phi1) * cos(phi2) *
               sin(deltaLambda / 2) * sin(deltaLambda / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));

    return R * c;
}

class MapControl : public QGraphicsView {
    Q_OBJECT

public:
    MapControl(QWidget* parent = nullptr) : QGraphicsView(parent), scene(new QGraphicsScene(this)) {
        setScene(scene);
        setDragMode(QGraphicsView::ScrollHandDrag);
        distanceLabel = new QLabel(this);
        distanceLabel->setStyleSheet("background: white; padding: 5px;");
        distanceLabel->move(10, 10);
        distanceLabel->setVisible(false);
    }

    void loadRaster(const std::string& filePath) {
        // Simplified raster loading (see earlier example)
        GDALDataset* dataset = (GDALDataset*)GDALOpen(filePath.c_str(), GA_ReadOnly);
        if (!dataset) {
            qWarning("Failed to load raster file");
            return;
        }

        // Convert raster to QImage and add to scene
        // Simplified for demonstration purposes
        QImage image(dataset->GetRasterXSize(), dataset->GetRasterYSize(), QImage::Format_RGB32);
        // Fill image with raster data (details omitted)

        scene->addPixmap(QPixmap::fromImage(image));
        GDALClose(dataset);
    }

    void loadShapefile(const std::string& filePath) {
          GDALDataset* dataset = (GDALDataset*)GDALOpenEx(filePath.c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr);
        if (!dataset) {
            qWarning("Failed to load shapefile");
            return;
        }

        OGRLayer* layer = dataset->GetLayer(0);
        if (!layer) {
            qWarning("Failed to get layer from shapefile");
            GDALClose(dataset);
            return;
        }

        OGRFeature* feature;
        while ((feature = layer->GetNextFeature()) != nullptr) {
            OGRGeometry* geom = feature->GetGeometryRef();
            if (geom && wkbFlatten(geom->getGeometryType()) == wkbPoint) {
                OGRPoint* point = (OGRPoint*)geom;
                scene->addEllipse(point->getX(), point->getY(), 5, 5, QPen(Qt::blue));
            }
            OGRFeature::DestroyFeature(feature);
        }

        GDALClose(dataset);
    }

    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            QPointF scenePos = mapToScene(event->pos());
            points.push_back(scenePos);

            if (points.size() > 1) {
                QPointF p1 = points[points.size() - 2];
                QPointF p2 = points[points.size() - 1];
                
                // Draw line between two points
                QGraphicsLineItem* line = scene->addLine(QLineF(p1, p2), QPen(Qt::red, 2));
                lines.push_back(line);

                // Calculate distance (dummy coordinates for demo, replace with real lat/lon if available)
                double distance = haversine(p1.x(), p1.y(), p2.x(), p2.y());
                totalDistance += distance;

                // Update distance label
                distanceLabel->setText(QString("Total Distance: %1 meters").arg(totalDistance));
                distanceLabel->setVisible(true);
            }

            // Mark point
            scene->addEllipse(scenePos.x() - 3, scenePos.y() - 3, 6, 6, QPen(Qt::blue), QBrush(Qt::blue));
        } else if (event->button() == Qt::RightButton) {
            clearMeasurement();
        }
    }

    void clearMeasurement() {
        // Clear all points, lines, and reset distance
        for (auto* line : lines) {
            scene->removeItem(line);
            delete line;
        }
        points.clear();
        lines.clear();
        totalDistance = 0;
        distanceLabel->setVisible(false);
    }

private:
    QGraphicsScene* scene;
    QLabel* distanceLabel;
    std::vector<QPointF> points;
    std::vector<QGraphicsLineItem*> lines;
    double totalDistance = 0.0;
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    MapControl mapControl;
    mapControl.loadRaster("path/to/raster.tif");
    mapControl.loadShapefile("path/to/shapefile.shp");

    mapControl.resize(800, 600);
    mapControl.show();

    return app.exec();
}
