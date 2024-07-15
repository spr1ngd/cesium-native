#include "CesiumRasterOverlays/QuadtreeRasterOverlayTileProvider.h"
#include <CesiumRasterOverlays/TileMapRasterOverlay.h>

using namespace CesiumAsync;
using namespace CesiumUtility;
using namespace CesiumGeospatial;
using namespace CesiumGeometry;

namespace CesiumRasterOverlays
{
    class TileMapTileProvider : public QuadtreeRasterOverlayTileProvider
    {
    public:

        TileMapTileProvider(
            const IntrusivePointer<const RasterOverlay>& pOwner,
            const AsyncSystem& asyncSystem,
            const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
            const std::shared_ptr<IPrepareRasterOverlayRendererResources>& pPrepareRendererResources,
            const std::shared_ptr<spdlog::logger>& pLogger,
            const CesiumGeospatial::Projection& projection,
            const CesiumGeometry::QuadtreeTilingScheme& tilingScheme,
            const CesiumGeometry::Rectangle& coverageRectangle,
            const std::string& url,
            const std::string& format,
            const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers,
            uint32_t minimumLevel,
            uint32_t maximumLevel,
            uint32_t width,
            uint32_t height,
            bool flipY,
            int8_t tileMapSrc
            ) : QuadtreeRasterOverlayTileProvider(pOwner, asyncSystem, pAssetAccessor, {}, pPrepareRendererResources, pLogger,
                    projection,
                    tilingScheme,
                    coverageRectangle,
                    minimumLevel, maximumLevel, width, height)
          , _url(url)
          , _format(format)
          , _headers(headers)
          , _flipY(flipY)
          , _tileMapSrc(tileMapSrc)
        {

        }

        virtual ~TileMapTileProvider() { }

    protected:

        // spell the url of tile
        CesiumAsync::Future<LoadedRasterOverlayImage> loadQuadtreeTileImage(const CesiumGeometry::QuadtreeTileID& tileID) const override
        {
            LoadTileImageFromUrlOptions options;
            options.allowEmptyImages = true;
            options.rectangle = this->getTilingScheme().tileToRectangle(tileID);
            options.moreDetailAvailable = tileID.level < this->getMaximumLevel();
            options.tileMapSource = this->_tileMapSrc;
            // options.rectangle = this->getTilingScheme().tileToRectangle(tileID);

            // const GlobeRectangle tileRectangle = unprojectRectangleSimple( this->getProjection(), options.rectangle);

            auto replacePlaceholder = [](const std::string& baseUrl, const std::string& placeholder, const std::string& value)
            {
                std::string result = baseUrl;
                size_t pos = result.find(placeholder);
                if (pos != std::string::npos) {
                  result.replace(pos, placeholder.length(), value);
                }
                return result;
            };

            const uint32_t z = tileID.level;
            const uint32_t x = tileID.x;
            const uint32_t y = _flipY == false ? tileID.y : (1u << tileID.level) - 1u - tileID.y;

            std::string url = _url;
            url = replacePlaceholder(url, "{z}", std::to_string(z));
            url = replacePlaceholder(url, "{x}", std::to_string(x));
            url = replacePlaceholder(url, "{y}", std::to_string(y));
            url += _format;
            return this->loadTileImageFromUrl(url, _headers, std::move(options));
        }

    private:

        std::string _url;
        std::string _format;
        std::vector<CesiumAsync::IAssetAccessor::THeader> _headers;
        bool _flipY;
        int8_t _tileMapSrc;
    };

    TileMapRasterOverlay::TileMapRasterOverlay (
        const std::string& name,
        const std::string& url,
        std::vector<CesiumAsync::IAssetAccessor::THeader> headers,
        const TileMapRasterOverlayOptions tmOptions,
        const RasterOverlayOptions overlayOptions)
          : RasterOverlay(name, overlayOptions)
          , _url(url)
          , _headers(headers)
          , _options(tmOptions)
    {

    }

    TileMapRasterOverlay::~TileMapRasterOverlay()
    {

    }

    CesiumAsync::Future<RasterOverlay::CreateTileProviderResult> TileMapRasterOverlay::createTileProvider (
        const CesiumAsync::AsyncSystem& asyncSystem,
        const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
        const std::shared_ptr<CesiumUtility::CreditSystem>& pCreditSystem,
        const std::shared_ptr<IPrepareRasterOverlayRendererResources>& pPrepareRendererResources,
        const std::shared_ptr<spdlog::logger>& pLogger,
        CesiumUtility::IntrusivePointer<const RasterOverlay> pOwner) const
    {
        pOwner = pOwner ? pOwner : this;

        auto promise = asyncSystem.createPromise<RasterOverlay::CreateTileProviderResult>();
        auto future = promise.getFuture();

        asyncSystem.runInMainThread([promise = std::move(promise), pOwner, asyncSystem, pAssetAccessor, pCreditSystem, pPrepareRendererResources, pLogger, this]() mutable
        {
            CesiumGeospatial::Projection projection;
            CesiumGeospatial::GlobeRectangle tilingSchemeRectangle = CesiumGeospatial::WebMercatorProjection::MAXIMUM_GLOBE_RECTANGLE;

            uint32_t rootTilesX = 1;
            if (_options.projection)
            {
                projection = _options.projection.value();
                if (std::get_if<CesiumGeospatial::GeographicProjection>(&projection))
                {
                    tilingSchemeRectangle = CesiumGeospatial::GeographicProjection::MAXIMUM_GLOBE_RECTANGLE;
                    rootTilesX = 2;
                }
            }
            else
            {
                if (_options.ellipsoid)
                {
                    projection = CesiumGeospatial::WebMercatorProjection(_options.ellipsoid.value());
                }
                else
                {
                    projection = CesiumGeospatial::WebMercatorProjection();
                }
            }

            CesiumGeometry::Rectangle coverageRectangle = _options.coverageRectangle.value_or(projectRectangleSimple(projection, tilingSchemeRectangle));

            CesiumGeometry::QuadtreeTilingScheme tilingScheme =
                _options.tilingScheme.value_or(CesiumGeometry::QuadtreeTilingScheme(
                    coverageRectangle,
                    rootTilesX,
                    1));

            CreateTileProviderResult handleResponseResult = new TileMapTileProvider(
                pOwner,
                asyncSystem,
                pAssetAccessor,
                pPrepareRendererResources,
                pLogger,
                projection,
                tilingScheme,
                coverageRectangle,
                this->_url,
                this->_options.format,
                this->_headers,
                (uint32_t)this->_options.minimumLevel,
                (uint32_t)this->_options.maximumLevel,
                (uint32_t)this->_options.tileWidth,
                (uint32_t)this->_options.tileHeight,
                this->_options.flipY,
                this->_options.tileMapSrc
            );
            promise.resolve(handleResponseResult);
        });
        return future;
    }
}
