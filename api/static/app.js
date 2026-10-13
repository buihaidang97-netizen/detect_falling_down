const BLYNK_TOKEN = "KveCGf33u8V1WhImVQS2E3YHGdjnCSYy";
const BLYNK_URL = "https://blynk.cloud/external/api";

// Protect route
if (!localStorage.getItem('adminToken')) {
    window.location.href = '/static/login.html';
}

// --- BLYNK API INTERACTION --- //

async function pushPin(pin, value) {
    try {
        await fetch(`${BLYNK_URL}/update?token=${BLYNK_TOKEN}&${pin}=${value}`);
    } catch (e) {
        console.error(`Error updating ${pin}:`, e);
    }
}

async function syncSlider(value) {
    document.getElementById('slider-val').innerText = value;
    await pushPin('V5', value);
}

async function fetchBlynkStatus() {
    try {
        // Fetch V1 (Terminal), V5 (Slider) từ Blynk và URL ảnh từ Python API
        const [v1Res, v5Res, apiRes] = await Promise.all([
            fetch(`${BLYNK_URL}/get?token=${BLYNK_TOKEN}&V1`),
            fetch(`${BLYNK_URL}/get?token=${BLYNK_TOKEN}&V5`),
            fetch('/api/status')
        ]);

        const v1 = await v1Res.text();
        const v5 = await v5Res.text();
        const apiData = await apiRes.json();

        // Update Terminal V1
        if (v1 && v1.length > 0) {
            // Replace newlines with <br> for HTML rendering
            const formattedText = v1.replace(/\n/g, '<br>');
            document.getElementById('terminal-content').innerHTML = formattedText;
            
            // Logic đổi màu nút
            const btnV0 = document.getElementById('btn-v0');
            const btnV11 = document.getElementById('btn-v11');
            const btnV12 = document.getElementById('btn-v12');

            // Reset tất cả về green
            btnV0.className = "blynk-btn green";
            btnV11.className = "blynk-btn green";
            btnV12.className = "blynk-btn green";

            if (v1.includes("CẢNH BÁO")) {
                // LEVEL 1: Nạn nhân tự cứu
                btnV0.className = "blynk-btn red";
                btnV12.className = "blynk-btn red";
            } else if (v1.includes("KHÔNG phản hồi")) {
                // LEVEL 2: Người nhà can thiệp
                btnV11.className = "blynk-btn red";
                btnV12.className = "blynk-btn red";
            }
        }

        // Update Slider V5 (only if not currently dragging to avoid jitter)
        const slider = document.getElementById('slider-v5');
        if (document.activeElement !== slider) {
            slider.value = v5;
            document.getElementById('slider-val').innerText = v5;
        }

        // Update Image từ API Python
        const imgEl = document.getElementById('evidence-img');
        const placeholder = document.getElementById('img-placeholder');
        
        let evidenceUrl = apiData.evidence_url;
        
        // Check if evidenceUrl is valid
        if (evidenceUrl && evidenceUrl.length > 5 && !evidenceUrl.includes('Transparent.gif')) {
            if (imgEl.src !== evidenceUrl) {
                imgEl.src = evidenceUrl;
            }
            placeholder.style.display = 'none';
        } else {
            imgEl.src = "data:image/gif;base64,R0lGODlhAQABAAD/ACwAAAAAAQABAAACADs=";
            placeholder.style.display = 'block';
        }

    } catch (e) {
        console.error("Error fetching Blynk status:", e);
    }
}

// Start polling Blynk API every 1 second
setInterval(fetchBlynkStatus, 1000);
fetchBlynkStatus(); // Initial fetch

// --- LOCAL API INTERACTION --- //

async function triggerMockFall() {
    try {
        const res = await fetch('/api/mock-fall', { method: 'POST' });
        const data = await res.json();
        alert("Đã gửi tín hiệu ngã giả lập thành công!\nVui lòng chờ 1-2 giây để hệ thống kích hoạt.");
    } catch (e) {
        alert("Lỗi khi giả lập ngã: " + e.message);
    }
}
