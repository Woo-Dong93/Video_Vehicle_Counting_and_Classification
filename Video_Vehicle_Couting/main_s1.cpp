#include "opencv.hpp"

using namespace cv;
using namespace std;

Mat BlendingPixel(const Mat& mbgrImgUnderlay, const Mat& mbgraImgOverlay, const Point& location); //�޴��� �׸��� �Լ�

#define ABS(x,y)   ((x)>(y) ? (x)-(y) : (y)-(x)) //Max or Min ����
#define RINFO_MAX    1280 // ȭ�� ������(Y)
#define RINFO_X      14 // �������� x��ǥ �̵���
#define RINFO_Y      14 // �������� y��ǥ �̵���
#define RINFO_VALID  515 // ���̻� ī��Ʈ���� �ʴ� ����(y)


bool check = false; // Line ���� ������ ���� ���ǽ�
int total_count = 0; // ī��Ʈ ����

typedef struct rInfo //������ ���Ͱ� ����� ���� ���� ����
{
	int x, y;
	int count;
	int px, py;
	int n;
	int id;
}   RINFO, *LPRINFO;

RINFO   rInfo[RINFO_MAX + 1];

LPRINFO   rInfoGet() //���� �ҷ�����
{
	return rInfo;
}

void   rInfoClear() //�ʱ�ȭ
{
	LPRINFO   p = rInfoGet();
	for (int i = 0; i<RINFO_MAX; i++)
	{
		p[i].x = p[i].y = p[i].count = 0;
		p[i].px = p[i].py = 0;
		p[i].n = p[i].id = 0;
	}
}

void   rInfoUpdate() // �ڵ����� ������ ������ ���ؼ� ����(������Ʈ)
{
	LPRINFO   p = rInfoGet();
	for (int i = 0; i<RINFO_MAX; i++)
	{
		if (p[i].n == 0)
			return;
		if (p[i].n == 1)
		{
			if (p[i].y < RINFO_VALID )
			{
				p[i].n = 2;
			}
		}
	}
}

int      rInfoPush(int x, int y) // ���� ��ǥ���� �������� �������� �Ǵ� �� ī��Ʈ
{
	LPRINFO   p = rInfoGet();
	int id = RINFO_MAX;
	int tx, ty;
	if (y > RINFO_VALID )
		return 2;

	for (int i = 0; i<RINFO_MAX; i++)
	{
		if (p[i].n == 0)
		{
			if (id == RINFO_MAX)
				id = i;
			break;
		}
		// ������ ����
		if (p[i].n == 2 && id == RINFO_MAX)
			id = i;

		tx = ABS(p[i].x, x);
		ty = ABS(p[i].y, y);
		if (ty < RINFO_Y && tx<RINFO_X)
		{

			//  ���� ����
			p[i].px = p[i].x;
			p[i].py = p[i].y;
			p[i].x = x;
			p[i].y = y;

			//	�Ȱ��� ���ο� �� �ٽ� �Ȱ��� ���� �� ��� ���� �������� �ν����� �ʰ� ����
			
			if (ty < 6 && tx <10)  //6 11
			{
				total_count += 1;
				p[id].x = x;
				p[id].y = y;
				p[id].px = 0;
				p[id].py = 0;
				p[id].n = 1;
				p[id].id = total_count;
				printf("\n                CAR ID = %d  @(%d.%d)\n", total_count, x, y);
				check = true;
			}
			
			

			return 0;


		}
	}
	// �ٸ��������� ������ ��� ī��Ʈ
	total_count += 1;
	p[id].x = x;
	p[id].y = y;
	p[id].px = 0;
	p[id].py = 0;
	p[id].n = 1;
	p[id].id = total_count;
	printf("\n                CAR ID = %d  @(%d.%d)\n", total_count, x, y);
	check = true;
	return 1;
}


int	main(int argc, char *argv[]) {

	VideoCapture capture("video1.mp4"); //������ ����

	rInfoClear(); // ī��Ʈ �ʱ�ȭ

	Mat frame; // Į��
	Mat Grayframe; // �׷���
	Mat old_frame; // ��������
	Mat sub_frame; // ����������
	Mat temp; // �������� ������
	Mat imgThresh; // Thresh ������
	Point crossingLine[2];  // ���� �ܱ� ����

	int count = 0; //ī��Ʈ ����

	Mat menu; //�޴� �׸� ����
	int car = 0; //car ī��Ʈ
	int bus = 0; // bus ī��Ʈ
	int truck = 0; // truck ī��Ʈ

	if (!capture.isOpened())
	{
		cout << " Can not open capture !!!" << endl;
		return 0;
	}

	int fps = (int)(capture.get(CAP_PROP_FPS));
	cout << "fps = " << fps << endl;

	int delay = 1000 / fps;


	for (;;)
	{
		capture >> frame;


		/////////////////// ���� �׸���
		crossingLine[0].x = 360;
		crossingLine[0].y = 502;
		crossingLine[1].x = 800;
		crossingLine[1].y = 502;
		line(frame, crossingLine[0], crossingLine[1], Scalar(0.0, 0.0, 255.0), 2);
		//////////////////


		if (frame.empty()) // �������� ������ �ߴ�
			break;

		cvtColor(frame, Grayframe, CV_BGR2GRAY); //2��ȭ

		GaussianBlur(Grayframe, Grayframe, Size(5, 5), 0); //GaussianBlur ����

		if (old_frame.empty()) //�� �������� �������
		{
			old_frame = Grayframe.clone(); // �� �������� �� �����ӿ� �ֱ�
			continue;
		}

		absdiff(old_frame, Grayframe, sub_frame); // ������

		threshold(sub_frame, imgThresh, 20, 255, CV_THRESH_BINARY); // Threshold -> ��������

		Mat element21(21, 21, CV_8U, cv::Scalar(1)); //�������� ������ ���� ��������
		morphologyEx(imgThresh, temp, cv::MORPH_CLOSE, element21);
		morphologyEx(temp, temp, cv::MORPH_OPEN, element21);

		vector<vector<Point> > contours;  // Convex Hull �ֿܰ��� ã��
		findContours(temp, contours, noArray(), CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
		vector<vector<Point> >hull(contours.size());

		for (int i = 0; i < contours.size(); i++)
		{
			convexHull(Mat(contours[i]), hull[i]);
		}

		Mat drawing = Mat::zeros(imgThresh.size(), CV_8U); // ���� ũ�� ������ ����
		drawContours(drawing, hull, -1, Scalar(255, 255, 255), -1); //�ֿܰ��������� �׸� �� �� ����ä���

		Mat img_labels, stats, centroids; // OpenCv 3.0���� �����ϴ� Labeling ����
		int numOfLables = connectedComponentsWithStats(drawing, img_labels, stats, centroids, 8, CV_32S);

		for (int j = 1; j < numOfLables; j++)
		{
			/////////////////////////////////////////////// ��ǥ�� ����
			int area = stats.at<int>(j, CC_STAT_AREA);
			int left = stats.at<int>(j, CC_STAT_LEFT);
			int top = stats.at<int>(j, CC_STAT_TOP);
			int width = stats.at<int>(j, CC_STAT_WIDTH);
			int height = stats.at<int>(j, CC_STAT_HEIGHT);

			int x = centroids.at<double>(j, 0);
			int y = centroids.at<double>(j, 1);
			///////////////////////////////////////////////

			if (width < 40 || height < 40) // ���� �簢������ ���͸�(����)
				continue;

			if (x < 400 || x>790) // x�� ���Ͱ� => �������������� ���͸�(����)
			{
				continue;
			}

			if (y < 502 || y >600) // y�� ���Ͱ� => �������������� ���͸�(����)
			{
				continue;
			}

			circle(frame, Point(x, y), 2, Scalar(255, 0, 0), 4); //���Ͱ��� ���׸���
			rectangle(frame, Point(left, top), Point(left + width, top + height), Scalar(255, 0, 0), 2); // ���Ͱ��� �߽����� �簢�� �׸���

			rInfoUpdate(); // �ڵ����� ������ ������ ���ؼ� ����(������Ʈ)
			rInfoPush(x, y); // �ڵ��� ī��Ʈ(���Ͱ��� �߽�����)

			if (check == true)  // ī��Ʈ�� ���� ���� ���� and �ڵ���, Ʈ��, ���� ũ�⺰�� ī��Ʈ
			{
				line(frame, crossingLine[0], crossingLine[1], Scalar(0.0, 255.0, 0.0), 2);
				if (area < 24000)
				{
					car++;
					cout << "size : ";
					cout << area;
					cout << endl;
				}
				else if (area > 24000)
				{
					truck++;
					cout << "size : ";
					cout << area;
					cout << endl;
				}
				else if (area > 30000)
				{
					bus++;
					cout << "size : ";
					cout << area;
					cout << endl;
				}
				check = false;
			}
		}


		//////////////////////////////////////////////////////////////////////////////// �޴��� �ռ�
		Mat mbgrImgSolGee = frame;
		Mat mbgraImgCatEars = cv::imread("menu1.png", ImreadModes::IMREAD_UNCHANGED);
		resize(mbgraImgCatEars, mbgraImgCatEars, Size(1280, 60));
		frame = BlendingPixel(mbgrImgSolGee, mbgraImgCatEars, Point(0, 0));


		putText(frame, to_string(bus), Point(200, 51), CV_FONT_HERSHEY_PLAIN, 4, Scalar(0, 0, 255), 5); //�۾��߰�_����
		putText(frame, to_string(truck), Point(515, 51), CV_FONT_HERSHEY_PLAIN, 4, Scalar(0, 0, 255), 5); //�۾��߰�_Ʈ��
		putText(frame, to_string(car), Point(820, 51), CV_FONT_HERSHEY_PLAIN, 4, Scalar(0, 0, 255), 5); //�۾��߰�_�ڵ���
		putText(frame, to_string(total_count), Point(1150, 51), CV_FONT_HERSHEY_PLAIN, 4, Scalar(0, 0, 255), 5); //�۾��߰�_����

		imshow("frame", frame);
//		imshow("sub_frame", sub_frame);
//		imshow("imgThresh", imgThresh);
//		imshow("drwaing", drawing);
//		imshow("gray", Grayframe);
		old_frame = Grayframe.clone(); //������������ ���������ӿ� �־ ����

		int ckey = waitKey(1);
		if (ckey == 27) break; // ESC ����
		if (ckey == 32) // Space �Ͻ�����
		{
			cvWaitKey(0);
		}
	}
	return 0;
}


Mat BlendingPixel(const Mat& mbgrImgUnderlay, const Mat& mbgraImgOverlay, const Point& location) // ����� �׸��� ��ġ�� ����ȭ �˰���(Menu �� �ռ�)
{
	Mat mbgrImgResult = mbgrImgUnderlay.clone();
	for (int y = max(location.y, 0); y < mbgrImgUnderlay.rows; ++y)
	{
		int fY = y - location.y;

		if (fY >= mbgraImgOverlay.rows)
			break;
		for (int x = std::max(location.x, 0); x < mbgrImgUnderlay.cols; ++x)
		{
			int fX = x - location.x;
			if (fX >= mbgraImgOverlay.cols)
				break;
			double opacity = 1; //�޴��� ����
			for (int c = 0; opacity > 0 && c < mbgrImgUnderlay.channels(); ++c)
			{
				unsigned char overlayPx = mbgraImgOverlay.data[fY * mbgraImgOverlay.step + fX * mbgraImgOverlay.channels() + c];
				unsigned char srcPx = mbgrImgUnderlay.data[y * mbgrImgUnderlay.step + x * mbgrImgUnderlay.channels() + c];
				mbgrImgResult.data[y * mbgrImgUnderlay.step + mbgrImgUnderlay.channels() * x + c] = srcPx * (1. - opacity) + overlayPx * opacity;
			}
		}
	}
	return mbgrImgResult;
}