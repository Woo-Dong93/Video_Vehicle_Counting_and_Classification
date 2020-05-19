#include "opencv.hpp"

using namespace cv;
using namespace std;

#define   ABS(x,y)   ((x)>(y) ? (x)-(y) : (y)-(x))

#define RINFO_MAX   1280
#define RINFO_X      18
#define RINFO_Y      18
#define RINFO_VALID  292

bool check = false;
int      total_count = 0;

typedef struct rInfo
{
	int x, y;
	int count;
	int px, py;
	int n;
	int id;
}   RINFO, *LPRINFO;

RINFO   rInfo[RINFO_MAX + 1];

LPRINFO   rInfoGet()
{
	return rInfo;
}

void   rInfoClear()
{
	LPRINFO   p = rInfoGet();
	for (int i = 0; i<RINFO_MAX; i++)
	{
		p[i].x = p[i].y = p[i].count = 0;
		p[i].px = p[i].py = 0;
		p[i].n = p[i].id = 0;
	}
}

void   rInfoUpdate()
{
	LPRINFO   p = rInfoGet();
	for (int i = 0; i<RINFO_MAX; i++)
	{
		if (p[i].n == 0)
			return;
		if (p[i].n == 1)
		{
			if (p[i].y < RINFO_VALID)
			{
				p[i].n = 2;
			}
		}
	}
}

int      rInfoPush(int x, int y)
{
	LPRINFO   p = rInfoGet();
	int id = RINFO_MAX;
	int tx, ty;
	if (y > RINFO_VALID)
		return 2;

	for (int i = 0; i<RINFO_MAX; i++)
	{
		if (p[i].n == 0)
		{
			if (id == RINFO_MAX)
				id = i;
			break;
		}
		//   deleted info
		if (p[i].n == 2 && id == RINFO_MAX)
			id = i;

		tx = ABS(p[i].x, x);
		ty = ABS(p[i].y, y);
		if ( ty < RINFO_Y && tx<RINFO_X)
		{
			
			//   same car
			p[i].px = p[i].x;
			p[i].py = p[i].y;
			p[i].x = x;
			p[i].y = y;
//			printf(" Updated %d::(%d.%d)", p[i].id, p[i].x, p[i].y);	
			if (ty < 11 && tx < 18)
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
	//   insert
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

	VideoCapture capture("video.mp4");

	rInfoClear();
	Mat frame;
	Mat Grayframe;
	Mat Grayframe1;
	Mat old_frame;
	Mat sub_frame;
	Mat temp;
	//	Mat fgMaskMOG2;
	//	Mat KNN;
	//	Mat imgThreshMog2;
	Mat imgThresh;
	Mat framethreshold;
	Point crossingLine[2];  //�߰�
	int count=0;
	int t = 0;

	Ptr<BackgroundSubtractor> pMOG2;
	//	Ptr<BackgroundSubtractor> pKNN;
	pMOG2 = createBackgroundSubtractorMOG2();
	//	pKNN = createBackgroundSubtractorKNN();


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




		///////////////////

		crossingLine[0].x = 0;
		crossingLine[0].y = 278;
		crossingLine[1].x = frame.cols - 1;
		crossingLine[1].y = 278;
		line(frame, crossingLine[0], crossingLine[1], Scalar(0.0, 0.0, 255.0), 2);
		//////////////////
	



		if (frame.empty())
			break;

	


		cvtColor(frame, Grayframe, CV_BGR2GRAY); //2��ȭ


												 //		 pMOG2->apply(Grayframe, fgMaskMOG2);
												 //		 pKNN->apply(Grayframe, KNN);
		GaussianBlur(Grayframe, Grayframe, Size(5, 5), 0);
	//	threshold(Grayframe, Grayframe, 90, 255, CV_THRESH_BINARY);
	//	framethreshold = Grayframe.clone();
//		medianBlur(Grayframe, Grayframe, 21);
	//	medianBlur(framethreshold, framethreshold, 3);





		if (old_frame.empty())
		{
			old_frame = Grayframe.clone(); //(������ ���� ��)�õ� �������� ������ �� �������� �־���, 
			continue; //�õ������ӿ� ������������ �ְ� �׳� ��Ƽ�� �״��� ���ο� �������� ������ ���������Ӱ� ������������ ȹ�������� ����
		}





		absdiff(old_frame, Grayframe, sub_frame);

		threshold(sub_frame, imgThresh, 20, 255, CV_THRESH_BINARY);

		//		Mat structuringElement5x5 = getStructuringElement(MORPH_RECT, Size(5, 5));  //���������������� ��ü��ġ��
		//		dilate(sub_frame, temp, structuringElement5x5);
		//		erode(temp, temp, structuringElement5x5);
		cv::Mat element5(21, 21, CV_8U, cv::Scalar(1));
		cv::morphologyEx(imgThresh, temp, cv::MORPH_CLOSE, element5);
		cv::morphologyEx(temp, temp, cv::MORPH_OPEN, element5);



		//		threshold(fgMaskMOG2, imgThreshMog2, 50, 255, CV_THRESH_BINARY);
		//		threshold(temp, imgThresh, 50, 255, CV_THRESH_BINARY);

		vector<vector<Point> > contours;  // �ֿܰ����׸��� ������� ä���
		findContours(temp, contours, noArray(), CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE); //�𼭶�ã��
		vector<vector<Point> >hull(contours.size());
		for (int i = 0; i < contours.size(); i++)

		{
			convexHull(Mat(contours[i]), hull[i]); //�ֿܰ���ã��
		}
		Mat drawing = Mat::zeros(imgThresh.size(), CV_8U);
		drawContours(drawing, hull, -1, Scalar(255, 255, 255), -1); //�ֿܰ��������� ������ �׸��� �� ����ä���




		Mat img_labels, stats, centroids;
		int numOfLables = connectedComponentsWithStats(drawing, img_labels, stats, centroids, 8, CV_32S);  //������� ä���� �ٰ����� �簢���׸���(��������)
		
	
		
	//	rptClear();      /////////////////////////////////////////////////////////////////////�߰�
		
		
		for (int j = 1; j < numOfLables; j++)
		{
		
			int area = stats.at<int>(j, CC_STAT_AREA);
			int left = stats.at<int>(j, CC_STAT_LEFT);
			int top = stats.at<int>(j, CC_STAT_TOP);
			int width = stats.at<int>(j, CC_STAT_WIDTH);
			int height = stats.at<int>(j, CC_STAT_HEIGHT);

			int x = centroids.at<double>(j, 0);
			int y = centroids.at<double>(j, 1);
			if (width < 40 || height < 40)
				continue;

			if (y < 278 || y >600)
			{
				continue;
			}

			circle(frame, Point(x, y), 2, Scalar(255, 0, 0), 4);
			rectangle(frame, Point(left, top), Point(left + width, top + height),
	     	Scalar(255, 0, 0), 2);		
	
//			if ((y>252) && (y<(252 + 12))) count++;
//			cout << count;
//			cout << endl;

//			if (y <= intHorizontalLinePosition)
//			{
//				count++;
//				cout << count;
//				cout << endl;
//			}
								
			rInfoUpdate();
			rInfoPush(x, y);
			if (check == true) {
				line(frame, crossingLine[0], crossingLine[1], Scalar(0.0, 255.0, 0.0), 2);
				check = false;
			}

			
				
	//		rptPush(left, top, width, height, j); /////////////////////////////////////////�߰�
			
		}
		
		putText(frame, to_string(total_count), Point(1190, 60), CV_FONT_HERSHEY_PLAIN, 4, Scalar(0, 0, 255), 5);
		putText(frame, "TOTAL=", Point(940, 60), CV_FONT_HERSHEY_PLAIN, 4, Scalar(0, 0, 255), 5);
		//cout << intHorizontalLinePosition;
		//cout << t;
		//cout << endl;

		//		rptFilter2();  ///////////////�߰�
		//   one more to clear noise
		//		rptFilter2(); //////////////�߰�

		//		rptDraw(frame); //////////////�߰�
		


		imshow("frame", frame);
		imshow("sub_frame", sub_frame);
		//		imshow("MOG2", fgMaskMOG2);
		//		imshow("KNN", KNN);
		//		imshow("imgThreshMog2", imgThreshMog2);
		imshow("imgThresh", imgThresh);
		imshow("drwaing", drawing);
		//imshow("temp", temp);
	//	imshow("framethreshold", framethreshold);
		imshow("gray", Grayframe);
		old_frame = Grayframe.clone(); //�ٽú���

		int ckey = waitKey(1);
		if (ckey == 27) break;
		if (ckey == 32) {
			cvWaitKey(0);
		}
	}
	return 0;
}

