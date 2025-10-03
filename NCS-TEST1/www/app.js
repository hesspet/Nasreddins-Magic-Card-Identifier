const videoElement = document.getElementById('cameraPreview');
const capturedImage = document.getElementById('capturedImage');
const statusMessage = document.getElementById('statusMessage');
const retakeButton = document.getElementById('retakeButton');

let activeStream;
let photoTaken = false;

async function startCamera() {
    stopCamera();
    photoTaken = false;
    capturedImage.hidden = true;
    videoElement.hidden = false;
    statusMessage.textContent = 'Starte Kamera… bitte Zugriff erlauben.';

    try {
        const constraints = {
            video: {
                facingMode: { ideal: 'environment' },
                width: { ideal: 1280 },
                height: { ideal: 720 }
            }
        };

        activeStream = await navigator.mediaDevices.getUserMedia(constraints);
        videoElement.srcObject = activeStream;

        videoElement.onloadedmetadata = () => {
            videoElement.play().catch(() => {});
        };

        videoElement.onplaying = () => {
            if (!photoTaken) {
                window.setTimeout(capturePhoto, 500);
            }
        };

        retakeButton.hidden = false;
        statusMessage.textContent = 'Kamera aktiv – Foto wird aufgenommen…';
    } catch (error) {
        statusMessage.textContent = 'Kamera konnte nicht gestartet werden: ' + error.message;
        console.error('Fehler beim Starten der Kamera', error);
    }
}

async function capturePhoto() {
    if (!activeStream) {
        return;
    }

    const [track] = activeStream.getVideoTracks();
    if (!track) {
        statusMessage.textContent = 'Kein Videostream verfügbar.';
        return;
    }

    try {
        let blob;
        if ('ImageCapture' in window) {
            const imageCapture = new ImageCapture(track);
            blob = await imageCapture.takePhoto();
        } else {
            const canvas = document.createElement('canvas');
            canvas.width = videoElement.videoWidth;
            canvas.height = videoElement.videoHeight;
            const context = canvas.getContext('2d');
            context.drawImage(videoElement, 0, 0, canvas.width, canvas.height);
            blob = await new Promise(resolve => canvas.toBlob(resolve, 'image/jpeg', 0.92));
        }

        if (!blob) {
            throw new Error('Foto konnte nicht erstellt werden.');
        }

        const imageUrl = URL.createObjectURL(blob);
        capturedImage.src = imageUrl;
        capturedImage.hidden = false;
        videoElement.hidden = true;
        statusMessage.textContent = 'Foto aufgenommen.';
        photoTaken = true;
    } catch (error) {
        statusMessage.textContent = 'Foto konnte nicht aufgenommen werden: ' + error.message;
        console.error('Fehler beim Fotografieren', error);
    } finally {
        stopCamera();
    }
}

function stopCamera() {
    if (activeStream) {
        activeStream.getTracks().forEach(track => track.stop());
        activeStream = null;
    }
}

retakeButton.addEventListener('click', () => {
    statusMessage.textContent = 'Neuer Versuch – Kamera wird erneut gestartet…';
    startCamera();
});

if (!('mediaDevices' in navigator) || !('getUserMedia' in navigator.mediaDevices)) {
    statusMessage.textContent = 'Diese Anwendung benötigt eine Kamera und wird von diesem Gerät/Browser nicht unterstützt.';
    retakeButton.hidden = true;
} else {
    startCamera();
}

window.addEventListener('beforeunload', stopCamera);
