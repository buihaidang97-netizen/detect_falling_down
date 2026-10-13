BẢN THUYẾT MINH SẢN PHẨM DỰ THI
CUỘC THI PHÁT TRIỂN SẢN PHẨM ỨNG DỤNG TRÍ TUỆ NHÂN TẠO (AI)

I. THÔNG TIN CHUNG
•	Tên sản phẩm:Smart Emergency Rescue System. 
•	Lĩnh vực ứng dụng: Đời sống  
•	Tên đội dự thi: The Silicon Soul
•	Thành viên nhóm: 
1.	Nguyễn Thị Bích Ngọc_BH02391_SE08201 
2.	Nguyễn Minh Ánh_BH02674_SE08203 
3.	Lê Duy Anh Đức_BH0693_SE08203 
4.	Bùi Hải Đăng_SE08201 
•	Đơn vị: Trường Cao đẳng Anh Quốc BTEC FPT 
•	Người hướng dẫn (nếu có): Ngô Thị Mai Loan 
II. MỞ ĐẦU
1. Ý tưởng hình thành sản phẩm
Hiện nay, công nghệ Internet of Things (IoT) và Trí tuệ nhân tạo (AI) đang được ứng dụng rộng rãi trong các hệ thống nhà thông minh và chăm sóc sức khỏe. Tai nạn té ngã là rủi ro sức khỏe nghiêm trọng đối với người cao tuổi, người có bệnh lý nền, trẻ em hoặc người sống một mình. Sự chậm trễ trong cấp cứu thường dẫn đến những biến chứng nặng nề.
Hiện nay, các giải pháp cảnh báo chủ yếu là thiết bị đeo vật lý (smartwatch, vòng cổ), có nhược điểm lớn là người dùng dễ quên đeo hoặc không thể tự thao tác bấm nút khi đã mất nhận thức.
Việc ứng dụng Trí tuệ nhân tạo (Computer Vision) thông qua camera giám sát giúp tạo ra một hệ thống "quan sát ẩn", không gây vướng víu, tự động phân tích tình huống và ra quyết định gọi hỗ trợ mà không cần sự can thiệp vật lý của nạn nhân.
2. Mục tiêu và lợi ích của sản phẩm
Mục tiêu: Xây dựng hệ thống tự động nhận diện hành vi té ngã thông qua phân tích tọa độ của  khớp xương với các góc và tốc độ thay đổi vị trí và thời gian bất động cảu đối tượng , kết hợp luồng xử lý cảnh báo đa cấp độ (tương tác giọng nói và cảnh báo người thân) để loại bỏ báo động giả và đảm bảo phát hiện sớm các ca nguy hiểm và đều được kết nối với y tế kịp thời .

Lợi ích: Tận dụng "thời gian vàng" trong cấp cứu y tế, mang lại sự an tâm cho gia đình và tối ưu hóa quy trình tiếp nhận thông tin của các cơ sở y tế (nhờ có sẵn tọa độ và hình ảnh hiện trường)

III. TIẾN TRÌNH THỰC HIỆN
1.	Chuẩn bị công nghệ / công cụ / tài nguyên
STT	Công nghệ/công cụ	Tài nguyên
1	Mô hình AI (Computer Vision)	YOLOv8-Pose hoặc MediaPipe Pose để trích xuất điểm neo (keypoints) và phân tích tư thế.
2	Nhận diện giọng nói (Speech-to-Text)	API Google Speech-to-Text để xử lý lệnh xác nhận tình trạng.
3	Phần cứng IoT	Vi điều khiển (ESP32), Camera module, Loa & Micro, Module GPS định vị.
4	Nền tảng quản lý & Cảnh báo	Ứng dụng Blynk để quản lý thông báo, API tích hợp tin nhắn SMS/Zalo.
5	Ngôn ngữ	C++ (ArduinoIDE) cho lập trình phần cứng ESP32, Python cho xử lý mô hình AI.  

2. Quy trình xây dựng sản phẩm
Bước 1: Thu thập dữ liệu và tùy chỉnh mô hình AI (YOLOv8/MediaPipe) để nhận diện chính xác sự thay đổi gia tốc tư thế và tính toán thời gian bất động.
Bước 2: Xây dựng luồng nghiệp vụ cảnh báo đa cấp độ (Level 1, Level 2) và lập trình kịch bản Chương trình SOS.
Bước 3: Tích hợp phần cứng IoT (vi điều khiển ESP32, loa, mic) với nền tảng ứng dụng Blynk.
Bước 4: Kiểm thử các tình huống thực tế (báo động giả, nạn nhân mất nhận thức, người nhà không cầm điện thoại) để gỡ lỗi.

3. Mô tả giải pháp và cách thức hoạt động
Phát hiện sự cố: Nhận diện người dùng thay đổi tư thế với gia tốc đột ngột (ngã) và duy trì dáng nằm bất động.
Level 1 (Nạn nhân còn nhận thức - Xử lý trong 2 phút đầu):
•	Hệ thống loa tự động phát giọng nói: "Bạn có ổn không?".
•	Người bị ngã tương tác bằng lệnh giọng nói. Nếu xác nhận "Tôi ổn", hủy cảnh báo. Nếu yêu cầu "Gọi cấp cứu", lập tức thực thi Chương trình SOS.
•	Đồng thời, đẩy một thông báo tức thời đến điện thoại người nhà qua ứng dụng Blynk.
Level 2 (Nạn nhân mất nhận thức / Không phản hồi trong 3 phút tiếp theo ):
•	Nếu sau 2 phút ở Level 1 không có phản hồi, hệ thống chuyển sang Level 2: Liên tục phát cảnh báo tại chỗ và gửi thông báo kèm hình ảnh/video qua Blynk cho người thân trong 3 phút tiếp theo.
•	Người thân có thể xác nhận trên App để hủy (nếu ngã nhẹ)  bằng nút bỏ qua hoặc kích hoạt ngay Chương trình SOS bằng nút SOS(thực hiện chương trình sos).
•	Fallback (Kịch bản an toàn cốt lõi): Nếu quá 5 phút kể từ lúc phát hiện ngã mà không có bất kỳ phản hồi nào (người thân không cầm điện thoại), hệ thống tự động kích hoạt Chương trình SOS.
Chương trình SOS: Tự động gửi tin nhắn khẩn cấp bao gồm: nội dung yêu cầu hỗ trợ, địa chỉ nhà cài sẵn, Số điện thoại người thân , tọa độ GPS hiện tại và hình ảnh/video hiện trường đến cơ sở y tế .
IV. ỨNG DỤNG TRÍ TUỆ NHÂN TẠO (AI)
1. Thành phần AI của hệ thống
•	Hệ thống Smart Emergency Rescue System sử dụng Module thị giác máy tính (Computer Vision) để phân tích tư thế, Module xử lý ngôn ngữ tự nhiên (Speech-to-Text) để nhận lệnh giọng nói.
•	Trí tuệ nhân tạo được tích hợp sâu vào Core xử lý hình ảnh (trên máy chủ/thiết bị Edge), cụ thể là module Nhận diện và Phân tích tư thế người (Human Pose Estimation). 
o	Đầu vào (Input): Luồng video trực tiếp từ Camera trong mô hình nhà thông minh. 
o	Đầu ra (Output): Trạng thái hành vi của người dùng (Đứng, Ngồi, Nằm, Té ngã, Bất động) và bộ tham số hình học cơ thể và đưa ra cảnh báo đến người dùng .
2. Mô tả ngắn gọn kỹ thuật AI áp dụng
•	Kỹ thuật áp dụng: Sử dụng YOLOv8-Pose/MediaPipe để theo dõi tọa độ 33 điểm khớp xương trên cơ thể người theo thời gian thực. Thuật toán tính toán đạo hàm sự thay đổi khoảng cách của các khớp so với mặt đất để xác định "tốc độ rơi".
•	Cách thức hoạt động: 
o	Mô hình thực hiện trích xuất 33 điểm mốc (keypoints) trên toàn bộ cơ thể người (bao gồm các khớp chính: vai, hông, đầu gối, cổ chân) theo thời gian thực (real-time). 
o	Hệ thống không lưu trữ hình ảnh gốc của người dùng mà chỉ trích xuất và xử lý tọa độ (x, y, z) của các điểm mốc này, đảm bảo tính riêng tư cao.
3. Vai trò của AI
AI Hoạt động như một "người giám sát y tế ảo", tự động hóa hoàn toàn việc phát hiện rủi ro và ra quyết định gọi hỗ trợ thay vì phụ thuộc vào khả năng vận động của nạn nhân.
AI sẽ dựa trên các cấp độ phân tích dựa trên các yếu tố thời gian và trạng thái của đối tượng để nhận biết các cấp độ nguy hiểm và kết hợp với phàn hồi từ của  nạn nhân hoặc người bảo hộ và triển khai các phương án được thiết lập. 
V. TÍNH MỚI VÀ TÍNH SÁNG TẠO CỦA SẢN PHẨM
1. Tính mới
•	Khác biệt hoàn toàn với các thiết bị đeo vướng víu hiện nay, giải pháp sử dụng phân tích hình ảnh không can thiệp vật lý lên người dùng. 
•	Cấu trúc logic cảnh báo 2 cấp độ và vòng lặp thời gian (timeout 5 phút) giải quyết vấn đề "báo động giả" và "người bảo hộ bỏ lỡ thông báo", tận dụng tối đa khoảng thời điểm vàng để tăng khả năng sống sót của bệnh nhân .
2. Tính sáng tạo
•	Kết hợp khéo léo giữa Computer Vision (nhìn) và Speech-to-Text (nghe) để tạo ra một hệ thống tương tác đa giác quan. 
•	Khả năng mở rộng dễ dàng thành một module tích hợp liền mạch vào các hệ thống nhà thông minh.
VI. KHẢ NĂNG ÁP DỤNG THỰC TIỄN
1. Phạm vi và đối tượng áp dụng
Đối tượng sử dụng sản phẩm.
•	Sản phẩm được thiết kế nhằm hỗ trợ giám sát và đảm bảo an toàn cho các đối tượng có nguy cơ cao gặp sự cố trong sinh hoạt hằng ngày, bao gồm:
•	Người cao tuổi sống một mình hoặc có sức khỏe yếu, dễ gặp các tai nạn như té ngã trong nhà.
•	Người sống một mình cần một hệ thống hỗ trợ theo dõi và cảnh báo khi xảy ra các tình huống nguy hiểm.
•	Trẻ nhỏ khi ở nhà mà không có người lớn giám sát trực tiếp trong một khoảng thời gian.
•	Người có tiền sử bệnh lý như tim mạch, huyết áp, đột quỵ hoặc các bệnh liên quan đến khả năng vận động và ý thức.
Môi trường hoặc bối cảnh áp dụng.
Sản phẩm có thể được triển khai trong nhiều môi trường khác nhau, bao gồm:
•	Nhà ở gia đình (Smart Home): hỗ trợ giám sát người thân khi họ ở nhà một mình.
•	Nhà dưỡng lão: giúp nhân viên theo dõi tình trạng người cao tuổi hiệu quả hơn.
•	Bệnh viện hoặc phòng chăm sóc bệnh nhân - hỗ trợ phát hiện sớm các tình huống nguy hiểm.
•	Căn hộ hoặc ký túc xá: dành cho người sống một mình cần một hệ thống giám sát an toàn cơ bản.
2. Hiệu quả dự kiến
•	Lợi ích xã hội và nhân văn: Tối ưu hóa thời gian cấp cứu, tân dung tối đa khoảng thời gian vàng bảo vệ tính mạng conngười. cấp cứu nhờ cơ chế tự động gửi tin nhắn SOS kèm vị trí và hình ảnh hiện trường.Nâng cao chất lượng cuộc sống và sự an tâm cho người cao tuổi, đồng thời giảm bớt gánh nặng tâm lý cho người thân trong gia đình.
•	Tính khả thi cao: triển khai rất cao do chi phí phần cứng rẻ, Tận dụng hạ tầng Camera và mạng Wi-Fi sẵn có, không phát sinh chi phí mua thêm cảm biến đeo tay hoặc thiết bị chuyên dụng đắt tiền. (sử dụng vi điều khiển và camera phổ thông), giao diện quản lý Blynk thân thiện với người dùng cuối
VII. HƯỚNG PHÁT TRIỂN TRONG TƯƠNG LAI
•	Tích hợp thiết bị có sẵn: Có thể liên kết với các thiết bị thông minh di động như đồng hồ thông minh và vòng đeo tay để nhận thêm các chỉ số kết hợp phân tích để tăng độ chính xác. 
•	Nâng cấp AI: Huấn luyện mô hình nhận diện thêm các dấu hiệu bất thường khác như ôm ngực (đau tim), loạng choạng (tiền đột quỵ). 
•	Hệ sinh thái thông minh: Đưa sản phẩm thành một tính năng tiêu chuẩn trong các giải pháp thiết kế thi công smart home trọn gói.
VIII. KẾT LUẬN
Smart Emergency Rescue System không chỉ là một ứng dụng công nghệ mà là một giải pháp hỗ trợ y tế dự phòng thiết thực. Với luồng nghiệp vụ chặt chẽ, chi phí tối ưu và tính nhân văn sâu sắc, sản phẩm sở hữu tiềm năng lớn để thương mại hóa và đóng góp giá trị bảo vệ sức khỏe cộng đồng.
IX. PHỤ LỤC (NẾU CÓ)

	Hà Nội , ngày 10  tháng 03 năm 2026
ĐẠI DIỆN NHÓM DỰ THI
(Ký và ghi rõ họ tên)
Ngọc
Nguyễn Thị Bích Ngọc



