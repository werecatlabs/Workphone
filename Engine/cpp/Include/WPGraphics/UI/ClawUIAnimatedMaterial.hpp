#ifndef CAnimatedMaterial_h__
#define CAnimatedMaterial_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/UI/IUIAnimatedMaterial.hpp>
#include <WPGraphics/UI/ClawUIElement.hpp>

namespace workphone
{
    namespace ui
    {
        class ClawUIAnimatedMaterial : public ClawUIElement<IUIAnimatedMaterial>
        {
        public:
            ClawUIAnimatedMaterial();
            ~ClawUIAnimatedMaterial() override;

            void update() override;

            void setMaterialName( const String &materialName ) override;
            String getMaterialName() const override;

            void play() override;
            void pause() override;
            void stop() override;

            void setPosition( const Vector2F &position ) override;

            void setSize( const Vector2F &size ) override;

            void draw( struct wp_context *ctx ) override;

        private:
            void setFrameTime( f32 time );
            f32 getFrameTime() const;
            // void setAnimationFunction(Ogre::ControllerFunctionRealPtr func);
            u32 getCurrentFrame() const;
            u32 getNumFrames() const;
            void OnEnterFrame() const;

            // class AnimationFunction : public Ogre::ControllerFunction<f32>
            //{
            // public:
            //	AnimationFunction(f32 sequenceTime, f32 timeOffset = 0.0f);

            //	/** Overridden function. */
            //	f32 calculate(f32 source);

            //	/** Set the time value manually. */
            //	void setTime(f32 timeVal);

            //	f32 getTime() const { return mTime; }

            //	/** Set the sequence duration value manually. */
            //	void setSequenceTime(f32 seqVal);

            //	bool isPlaying() const { return m_isPlaying; }
            //	void setIsPlaying(bool newValue) { m_isPlaying = newValue; }

            //	bool getLoop() const { return m_loop; }
            //	void setLoop(bool newValue) { m_loop = newValue; }

            // protected:
            //	f32 mSeqTime;
            //	f32 mTime;
            //	bool m_isPlaying;
            //	bool m_loop;
            // };

            String m_materialName;
            f32 m_time = 0.0f;
            f32 m_frameTime = 1.0f / 12.0f;
            u32 m_currentFrame = 0;
            u32 m_numFrames = 1;
            bool m_isPlaying = false;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // CAnimatedMaterial_h__
