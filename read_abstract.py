
import docx
import sys

def read_docx(file_path):
    try:
        doc = docx.Document(file_path)
        fullText = []
        for para in doc.paragraphs:
            fullText.append(para.text)
        return '\n'.join(fullText)
    except Exception as e:
        return str(e)

if __name__ == "__main__":
    path = r"D:\GitHub\Human-Falling-Detect-Tracks\abstract dự án phát hiện người ngã và tự động phản ứng theo kịch bản.docx"
    print(read_docx(path))
