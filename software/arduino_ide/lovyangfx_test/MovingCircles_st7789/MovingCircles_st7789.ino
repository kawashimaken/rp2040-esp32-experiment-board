
#include <LovyanGFX.hpp>

// ESP32でLovyanGFXを独自設定で利用する場合の設定例

/// 独自の設定を行うクラスを、LGFX_Deviceから派生して作成します。
class LGFX : public lgfx::LGFX_Device {
  /*
 クラス名は"LGFX"から別の名前に変更しても構いません。
 AUTODETECTと併用する場合は"LGFX"は使用されているため、LGFX以外の名前に変更してください。
 また、複数枚のパネルを同時使用する場合もそれぞれに異なる名前を付けてください。
 ※ クラス名を変更する場合はコンストラクタの名前も併せて同じ名前に変更が必要です。

 名前の付け方は自由に決めて構いませんが、設定が増えた場合を想定し、
 例えばESP32 DevKit-CでSPI接続のILI9341の設定を行った場合、
  LGFX_DevKitC_SPI_ILI9341
 のような名前にし、ファイル名とクラス名を一致させておくことで、利用時に迷いにくくなります。
//*/

  // 接続するパネルの型にあったインスタンスを用意します。
  lgfx::Panel_ST7789 _panel_instance;

  // パネルを接続するバスの種類にあったインスタンスを用意します。
  lgfx::Bus_SPI _bus_instance;  // SPIバスのインスタンス

public:

  // コンストラクタを作成し、ここで各種設定を行います。
  // クラス名を変更した場合はコンストラクタも同じ名前を指定してください。
  LGFX(void) {
    {                                     // バス制御の設定を行います。
      auto cfg = _bus_instance.config();  // バス設定用の構造体を取得します。

      // SPIバスの設定
      cfg.spi_host = VSPI_HOST;  // 使用するSPIを選択  ESP32-S2,C3 : SPI2_HOST or SPI3_HOST / ESP32 : VSPI_HOST or HSPI_HOST
      // ※ ESP-IDFバージョンアップに伴い、VSPI_HOST , HSPI_HOSTの記述は非推奨になるため、エラーが出る場合は代わりにSPI2_HOST , SPI3_HOSTを使用してください。
      cfg.spi_mode =3;                   // SPI通信モードを設定 (0 ~ 3)
      cfg.freq_write = 40000000;          // 送信時のSPIクロック (最大80MHz, 80MHzを整数で割った値に丸められます)
      cfg.freq_read = 16000000;           // 受信時のSPIクロック
      cfg.spi_3wire = true;               // 受信をMOSIピンで行う場合はtrueを設定
      cfg.use_lock = true;                // トランザクションロックを使用する場合はtrueを設定
      cfg.dma_channel = SPI_DMA_CH_AUTO;  // 使用するDMAチャンネルを設定 (0=DMA不使用 / 1=1ch / 2=ch / SPI_DMA_CH_AUTO=自動設定)
      // ※ ESP-IDFバージョンアップに伴い、DMAチャンネルはSPI_DMA_CH_AUTO(自動設定)が推奨になりました。1ch,2chの指定は非推奨になります。
      cfg.pin_sclk = 14;  // SPIのSCLKピン番号を設定
      cfg.pin_mosi = 13;  // SPIのMOSIピン番号を設定
      cfg.pin_miso = -1;  // SPIのMISOピン番号を設定 (-1 = disable)
      cfg.pin_dc = 5;     // SPIのD/Cピン番号を設定  (-1 = disable)
                          // SDカードと共通のSPIバスを使う場合、MISOは省略せず必ず設定してください。
                          //*/


      _bus_instance.config(cfg);               // 設定値をバスに反映します。
      _panel_instance.setBus(&_bus_instance);  // バスをパネルにセットします。
    }

    {                                       // 表示パネル制御の設定を行います。
      auto cfg = _panel_instance.config();  // 表示パネル設定用の構造体を取得します。

      cfg.pin_cs = -1;    // CSが接続されているピン番号   (-1 = disable)
      cfg.pin_rst = 15;   // RSTが接続されているピン番号  (-1 = disable)
      cfg.pin_busy = -1;  // BUSYが接続されているピン番号 (-1 = disable)

      // ※ 以下の設定値はパネル毎に一般的な初期値が設定されていますので、不明な項目はコメントアウトして試してみてください。
      cfg.panel_width = 240;     // 実際に表示可能な幅
      cfg.panel_height = 240;    // 実際に表示可能な高さ
      cfg.offset_x = 0;          // パネルのX方向オフセット量
      cfg.offset_y = 0;          // パネルのY方向オフセット量
      cfg.offset_rotation = 0;   // 回転方向の値のオフセット 0~7 (4~7は上下反転)
      cfg.dummy_read_pixel = 8;  // ピクセル読出し前のダミーリードのビット数
      cfg.dummy_read_bits = 1;   // ピクセル以外のデータ読出し前のダミーリードのビット数
      cfg.readable = true;       // データ読出しが可能な場合 trueに設定
      cfg.invert = true;        // パネルの明暗が反転してしまう場合 trueに設定
      cfg.rgb_order = false;     // パネルの赤と青が入れ替わってしまう場合 trueに設定
      cfg.dlen_16bit = false;    // 16bitパラレルやSPIでデータ長を16bit単位で送信するパネルの場合 trueに設定
      cfg.bus_shared = false;     // SDカードとバスを共有している場合 trueに設定(drawJpgFile等でバス制御を行います)

      _panel_instance.config(cfg);
    }

    setPanel(&_panel_instance);  // 使用するパネルをセットします。
  }
};

static LGFX lcd;

static std::uint32_t sec, psec;
static std::uint32_t fps = 0, frame_count = 0;
static std::int32_t lcd_width;
static std::int32_t lcd_height;

struct obj_info_t {
  std::int32_t x;
  std::int32_t y;
  std::int32_t r;
  std::int32_t dx;
  std::int32_t dy;
  std::uint32_t color;

  void move() {
    x += dx;
    y += dy;
    if (x < 0) {
      x = 0;
      if (dx < 0) dx = -dx;
    } else if (x >= lcd_width) {
      x = lcd_width - 1;
      if (dx > 0) dx = -dx;
    }
    if (y < 0) {
      y = 0;
      if (dy < 0) dy = -dy;
    } else if (y >= lcd_height) {
      y = lcd_height - 1;
      if (dy > 0) dy = -dy;
    }
  }
};

static constexpr std::uint32_t obj_count = 200;
static obj_info_t objects[obj_count];

static LGFX_Sprite sprites[2];
static int_fast16_t sprite_height;

void setup(void) {
  lcd.init();

  if (lcd.width() < lcd.height()) {
    lcd.setRotation(lcd.getRotation() ^ 1);
  }
  lcd_width = lcd.width();
  lcd_height = lcd.height();
  obj_info_t *a;
  for (std::uint32_t i = 0; i < obj_count; ++i) {
    a = &objects[i];
    a->color = (uint32_t)rand() | 0x808080U;
    a->x = rand() % lcd_width;
    a->y = rand() % lcd_height;
    a->dx = ((rand() & 3) + 1) * (i & 1 ? 1 : -1);
    a->dy = ((rand() & 3) + 1) * (i & 2 ? 1 : -1);
    a->r = 8 + (i & 0x07);
  }

  uint32_t div = 2;
  for (;;) {
    sprite_height = (lcd_height + div - 1) / div;
    bool fail = false;
    for (std::uint32_t i = 0; !fail && i < 2; ++i) {
      sprites[i].setColorDepth(lcd.getColorDepth());
      sprites[i].setFont(&fonts::Font2);
      fail = !sprites[i].createSprite(lcd_width, sprite_height);
    }
    if (!fail) break;
    for (std::uint32_t i = 0; i < 2; ++i) {
      sprites[i].deleteSprite();
    }
    ++div;
  }
  lcd.startWrite();
}

void loop(void) {
  static std::uint_fast8_t flip = 0;

  obj_info_t *a;
  for (std::uint32_t i = 0; i != obj_count; i++) {
    objects[i].move();
  }
  for (std::int32_t y = 0; y < lcd_height; y += sprite_height) {
    flip = flip ? 0 : 1;
    sprites[flip].clear();
    for (std::uint32_t i = 0; i != obj_count; i++) {
      a = &objects[i];
      if ((a->y + a->r >= y) && (a->y - a->r <= y + sprite_height))
        sprites[flip].drawCircle(a->x, a->y - y, a->r, a->color);
    }

    if (y == 0) {
      sprites[flip].setCursor(1, 1);
      sprites[flip].setTextColor(TFT_BLACK);
      sprites[flip].printf("obj:%d fps:%d", obj_count, fps);
      sprites[flip].setCursor(0, 0);
      sprites[flip].setTextColor(TFT_WHITE);
      sprites[flip].printf("obj:%d fps:%d", obj_count, fps);
    }
    sprites[flip].pushSprite(&lcd, 0, y);
  }
  lcd.display();

  ++frame_count;
  sec = lgfx::millis() / 1000;
  if (psec != sec) {
    psec = sec;
    fps = frame_count;
    frame_count = 0;
  }
}
