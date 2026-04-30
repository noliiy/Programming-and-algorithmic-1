#include <stdio.h>
#include <math.h>

int main() {
    double x1, y1, r1, x2, y2, r2;
    int scan1, scan2;
    
    printf("Enter circle #1 parameters:\n");
    scan1 = scanf("%lf %lf %lf", &x1, &y1, &r1);
    
    if (scan1 != 3) {
        printf("Invalid input.\n");
        return 0;
    }
    
    if (r1 <= 0) {
        printf("Invalid input.\n");
        return 0;
    }
    
    printf("Enter circle #2 parameters:\n");
    scan2 = scanf("%lf %lf %lf", &x2, &y2, &r2);
    
    if (scan2 != 3) {
        printf("Invalid input.\n");
        return 0;
    }
    
    if (r2 <= 0) {
        printf("Invalid input.\n");
        return 0;
    }
    
    double dx = x2 - x1;
    double dy = y2 - y1;
    double distance = sqrt(dx * dx + dy * dy);
    if (x1 == x2 && y1 == y2 && r1 == r2) {
        double area = 3.14159265359 * r1 * r1;
        printf("The circles are identical, overlap: %.6f\n", area);
        return 0;
    }
    
    double diff = fabs(r1 - r2);
    double sum = r1 + r2;
    // Fixed epsilon (applied after simple scale normalization)
    double eps = 1e-7;
    double scale = fmax(fabs(distance), fmax(fabs(r1), fabs(r2)));
    if (scale == 0.0) scale = 1.0; // safety
    double ds = distance / scale;
    double sums = sum / scale;
    double diffs = diff / scale;

    // External touch (d ~= r1 + r2)
    if (fabs(ds - sums) <= eps) {
        printf("External touch, no overlap.\n");
        return 0;
    }

    // Internal touch (d ~= |r1 - r2|)
    if (fabs(ds - diffs) <= eps) {
        if (r1 >= r2) {
            double area = 3.14159265359 * r2 * r2;
            printf("Internal touch, circle #2 lies inside circle #1, overlap: %.6f\n", area);
        } else {
            double area = 3.14159265359 * r1 * r1;
            printf("Internal touch, circle #1 lies inside circle #2, overlap: %.6f\n", area);
        }
        return 0;
    }

    // One circle strictly inside the other (d < |r1 - r2|)
    if (ds < diffs - eps) {
        if (r1 > r2) {
            double area = 3.14159265359 * r2 * r2;
            printf("Circle #2 lies inside circle #1, overlap: %.6f\n", area);
        } else {
            double area = 3.14159265359 * r1 * r1;
            printf("Circle #1 lies inside circle #2, overlap: %.6f\n", area);
        }
        return 0;
    }

    // Intersect (|r1 - r2| < d < r1 + r2)
    if (ds < sums - eps) {
        double r1_squared = r1 * r1;
        double r2_squared = r2 * r2;
        double distance_squared = distance * distance;
        
        double cos1 = (distance_squared + r1_squared - r2_squared) / (2 * distance * r1);
        double cos2 = (distance_squared + r2_squared - r1_squared) / (2 * distance * r2);
        
        if (cos1 > 1.0) cos1 = 1.0;
        if (cos1 < -1.0) cos1 = -1.0;
        if (cos2 > 1.0) cos2 = 1.0;
        if (cos2 < -1.0) cos2 = -1.0;
        
        double a1 = r1_squared * acos(cos1);
        double a2 = r2_squared * acos(cos2);
        double a3 = 0.5 * sqrt((-distance + r1 + r2) * (distance + r1 - r2) * (distance - r1 + r2) * (distance + r1 + r2));
        
        double overlap = a1 + a2 - a3;
        printf("The circles intersect, overlap: %.6f\n", overlap);
        return 0;
    }
    
    printf("The circles lie outside each other, no overlap.\n");
    
    return 0;
}