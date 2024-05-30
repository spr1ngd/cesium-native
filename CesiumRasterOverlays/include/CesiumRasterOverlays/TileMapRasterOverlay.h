#pragma once
#include "Library.h"
#include "RasterOverlay.h"
#include "CesiumGeometry/Rectangle.h"
#include <CesiumGeometry/QuadtreeTilingScheme.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/Projection.h>
#include <optional>
#include <string>

namespace CesiumRasterOverlays
{
    struct TileMapRasterOverlayOptions
    {
        std::string format = "png";
        int32_t minimumLevel = 0;
        int32_t maximumLevel = 14;
        int32_t tileWidth = 256;
        int32_t tileHeight = 256;
        std::optional<CesiumGeospatial::Projection> projection;
        std::optional<CesiumGeometry::Rectangle> coverageRectangle;
        std::optional<CesiumGeometry::QuadtreeTilingScheme> tilingScheme;
        std::optional<CesiumGeospatial::Ellipsoid> ellipsoid;
        bool flipY = false;
    };

    class CESIUMRASTEROVERLAYS_API TileMapRasterOverlay : public RasterOverlay
    {
    public:

        TileMapRasterOverlay(
            const std::string& name,
            const std::string& url,
            std::vector<CesiumAsync::IAssetAccessor::THeader> headers = {},
            const TileMapRasterOverlayOptions tmOptions = {},
            const RasterOverlayOptions overlayOptions = {}
        );

        virtual ~TileMapRasterOverlay() override;

        virtual CesiumAsync::Future<CreateTileProviderResult> createTileProvider(
            const CesiumAsync::AsyncSystem& asyncSystem,
            const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
            const std::shared_ptr<CesiumUtility::CreditSystem>& pCreditSystem,
            const std::shared_ptr<IPrepareRasterOverlayRendererResources>& pPrepareRendererResources,
            const std::shared_ptr<spdlog::logger>& pLogger,
            CesiumUtility::IntrusivePointer<const RasterOverlay> pOwner
        ) const override;

    private:

        std::string _url;
        std::vector<CesiumAsync::IAssetAccessor::THeader> _headers;
        TileMapRasterOverlayOptions _options;
    };
}
