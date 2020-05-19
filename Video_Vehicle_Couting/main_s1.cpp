#include "opencv.hpp"

using namespace cv;
using namespace std;

Mat BlendingPixel(const Mat& mbgrImgUnderlay, const Mat& mbgraImgOverlay, const Point& location); //메뉴바 그리기 함수

#define ABS(x,y)   ((x)>(y) ? (x)-(y) : (y)-(x)) //Max or Min 변경
#define RINFO_MAX    1280 // 화면 사이즈(Y)
#define RINFO_X      14 // 동일차량 x좌표 이동값
#define RINFO_Y      14 // 동일차량 y좌표 이동값
#define RINFO_VALID  515 // 더이상 카운트하지 않는 범위(y)


bool check = false; // Line 색상 변경을 위한 조건식
int total_count = 0; // 카운트 변수

typedef struct rInfo //차량의 센터값 계산을 위한 정보 저장
{
	int x, y;
	int count;
	int px, py;
	int n;
	int id;
}   RINFO, *LPRINFO;

RINFO   rInfo[RINFO_MAX + 1];

LPRINFO   rInfoGet() //정보 불러오기
{
	return rInfo;
}

void   rInfoClear() //초기화
{
	LPRINFO   p = rInfoGet();
	for (int i = 0; i<RINFO_MAX; i++)
	{
		p[i].x = p[i].y = p[i].count = 0;
		p[i].px = p[i].py = 0;
		p[i].n = p[i].id = 0;
	}
}

void   rInfoUpdate() // 자동차가 지나간 영역에 대해서 삭제(업데이트)
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

int      rInfoPush(int x, int y) // 센터 좌표값을 바탕으로 동일차량 판단 후 카운트
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
		// 삭제된 정보
		if (p[i].n == 2 && id == RINFO_MAX)
			id = i;

		tx = ABS(p[i].x, x);
		ty = ABS(p[i].y, y);
		if (ty < RINFO_Y && tx<RINFO_X)
		{

			//  같은 차량
			p[i].px = p[i].x;
			p[i].py = p[i].y;
			p[i].x = x;
			p[i].y = y;

			//	똑같은 라인에 또 다시 똑같은 차가 올 경우 같은 차량으로 인식하지 않게 설정
			
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
	// 다른차량으로 생각될 경우 카운트
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

	VideoCapture capture("video1.mp4"); //두정역 육교

	rInfoClear(); // 카운트 초기화

	Mat frame; // 칼라
	Mat Grayframe; // 그레이
	Mat old_frame; // 전프레임
	Mat sub_frame; // 현재프레임
	Mat temp; // 모폴로지 프레임
	Mat imgThresh; // Thresh 프레임
	Point crossingLine[2];  // 라인 긁기 변수

	int count = 0; //카운트 변수

	Mat menu; //메뉴 그림 변수
	int car = 0; //car 카운트
	int bus = 0; // bus 카운트
	int truck = 0; // truck 카운트

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


		/////////////////// 직선 그리기
		crossingLine[0].x = 360;
		crossingLine[0].y = 502;
		crossingLine[1].x = 800;
		crossingLine[1].y = 502;
		line(frame, crossingLine[0], crossingLine[1], Scalar(0.0, 0.0, 255.0), 2);
		//////////////////


		if (frame.empty()) // 프레임이 없으면 중단
			break;

		cvtColor(frame, Grayframe, CV_BGR2GRAY); //2진화

		GaussianBlur(Grayframe, Grayframe, Size(5, 5), 0); //GaussianBlur 적용

		if (old_frame.empty()) //전 프레임이 없을경우
		{
			old_frame = Grayframe.clone(); // 현 프레임을 전 프레임에 넣기
			continue;
		}

		absdiff(old_frame, Grayframe, sub_frame); // 차영상

		threshold(sub_frame, imgThresh, 20, 255, CV_THRESH_BINARY); // Threshold -> 잡음제거

		Mat element21(21, 21, CV_8U, cv::Scalar(1)); //모폴로지 연산을 통한 잡음제거
		morphologyEx(imgThresh, temp, cv::MORPH_CLOSE, element21);
		morphologyEx(temp, temp, cv::MORPH_OPEN, element21);

		vector<vector<Point> > contours;  // Convex Hull 최외곽선 찾기
		findContours(temp, contours, noArray(), CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
		vector<vector<Point> >hull(contours.size());

		for (int i = 0; i < contours.size(); i++)
		{
			convexHull(Mat(contours[i]), hull[i]);
		}

		Mat drawing = Mat::zeros(imgThresh.size(), CV_8U); // 같은 크리 프레임 생성
		drawContours(drawing, hull, -1, Scalar(255, 255, 255), -1); //최외각선값으로 그린 후 그 영역채우기

		Mat img_labels, stats, centroids; // OpenCv 3.0에서 지원하는 Labeling 구현
		int numOfLables = connectedComponentsWithStats(drawing, img_labels, stats, centroids, 8, CV_32S);

		for (int j = 1; j < numOfLables; j++)
		{
			/////////////////////////////////////////////// 좌표값 저장
			int area = stats.at<int>(j, CC_STAT_AREA);
			int left = stats.at<int>(j, CC_STAT_LEFT);
			int top = stats.at<int>(j, CC_STAT_TOP);
			int width = stats.at<int>(j, CC_STAT_WIDTH);
			int height = stats.at<int>(j, CC_STAT_HEIGHT);

			int x = centroids.at<double>(j, 0);
			int y = centroids.at<double>(j, 1);
			///////////////////////////////////////////////

			if (width < 40 || height < 40) // 작은 사각형들은 필터링(제거)
				continue;

			if (x < 400 || x>790) // x는 센터값 => 일정범위에서는 필터링(제거)
			{
				continue;
			}

			if (y < 502 || y >600) // y는 센터값 => 일정범위에서는 필터링(제거)
			{
				continue;
			}

			circle(frame, Point(x, y), 2, Scalar(255, 0, 0), 4); //센터값에 원그리기
			rectangle(frame, Point(left, top), Point(left + width, top + height), Scalar(255, 0, 0), 2); // 센터값을 중심으로 사각형 그리기

			rInfoUpdate(); // 자동차가 지나간 영역에 대해서 삭제(업데이트)
			rInfoPush(x, y); // 자동차 카운트(센터값을 중심으로)

			if (check == true)  // 카운트시 직선 색상 변경 and 자동차, 트럭, 버스 크기별로 카운트
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


		//////////////////////////////////////////////////////////////////////////////// 메뉴바 합성
		Mat mbgrImgSolGee = frame;
		Mat mbgraImgCatEars = cv::imread("menu1.png", ImreadModes::IMREAD_UNCHANGED);
		resize(mbgraImgCatEars, mbgraImgCatEars, Size(1280, 60));
		frame = BlendingPixel(mbgrImgSolGee, mbgraImgCatEars, Point(0, 0));


		putText(frame, to_string(bus), Point(200, 51), CV_FONT_HERSHEY_PLAIN, 4, Scalar(0, 0, 255), 5); //글씨추가_버스
		putText(frame, to_string(truck), Point(515, 51), CV_FONT_HERSHEY_PLAIN, 4, Scalar(0, 0, 255), 5); //글씨추가_트럭
		putText(frame, to_string(car), Point(820, 51), CV_FONT_HERSHEY_PLAIN, 4, Scalar(0, 0, 255), 5); //글씨추가_자동차
		putText(frame, to_string(total_count), Point(1150, 51), CV_FONT_HERSHEY_PLAIN, 4, Scalar(0, 0, 255), 5); //글씨추가_총합

		imshow("frame", frame);
//		imshow("sub_frame", sub_frame);
//		imshow("imgThresh", imgThresh);
//		imshow("drwaing", drawing);
//		imshow("gray", Grayframe);
		old_frame = Grayframe.clone(); //현재프레임을 이전프레임에 넣어서 변경

		int ckey = waitKey(1);
		if (ckey == 27) break; // ESC 종료
		if (ckey == 32) // Space 일시정지
		{
			cvWaitKey(0);
		}
	}
	return 0;
}


Mat BlendingPixel(const Mat& mbgrImgUnderlay, const Mat& mbgraImgOverlay, const Point& location) // 영상과 그림을 합치는 최적화 알고리즘(Menu 바 합성)
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
			double opacity = 1; //메뉴바 투명도
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